#include "Binarization.hpp"

namespace DaVinci
{

	void Binarization::ImgBinarization(const Mat& src, Mat& dst, BinarizationType BT)
	{
		assert(BT==BinarizationType::OTSU||BT==BinarizationType::NIBLACK);
		switch (BT)
		{
		case(BinarizationType::NIBLACK):
			{
				float delta = 0.5;
				int winy = (int) (2.0 * src.rows-1)/3;
				int winx = (int) src.cols-1 < winy ? src.cols-1 : winy;
				winx = winx/2;
				winy = winy/2;
				if(winx%2 == 0)
					winx += 1;
				if(winy%2 == 0)
				{
					winy += 1;
				}
				if (winx > 21) winx = winy = 21;

				Mat gray;
				if(3==src.channels())
				{
					cvtColor(src,gray,CV_BGR2GRAY);
				}
				else
				{
					gray = src;
				}
				NiBlackBinarization(gray,dst,255,THRESH_BINARY,Size(winx,winy),delta);
				break;
			}
		default:
			{
				Mat gray;;
				if(3==src.channels())
				{
					cvtColor(src,gray,CV_BGR2GRAY);
				}
				else
				{
					gray = src;
				}
				threshold(src,dst,1,255,THRESH_OTSU);
				break;
			}
		}
	}

	void Binarization::NiBlackBinarization(const Mat& src, Mat& dst, double maxValue,
		int type, Size blockSize,double delta)
	{
		CV_Assert(src.channels() == 1);
		CV_Assert(blockSize.width % 2 == 1 && blockSize.width > 1);
		CV_Assert(blockSize.height % 2 == 1 && blockSize.height > 1);
		type &= THRESH_MASK;

		// Compute local threshold (T = mean + k * stddev)
		// using mean and standard deviation in the neighborhood of each pixel
		// (intermediate calculations are done with floating-point precision)
		Mat thresh;
		{
			// note that: Var[X] = E[X^2] - E[X]^2
			Mat mean, sqmean, stddev;
			boxFilter(src, mean, CV_32F, blockSize,
				Point(-1,-1), true, BORDER_REPLICATE);
			Mat srcF;
			src.convertTo(srcF,CV_32FC1);
			Mat sqrSrc;
			sqrSrc = srcF.mul(srcF);
			boxFilter(sqrSrc, sqmean, CV_32F, blockSize,
				Point(-1,-1), true, BORDER_REPLICATE);
			sqrt(sqmean - mean.mul(mean), stddev);
			thresh = mean + stddev * static_cast<float>(delta);
			thresh.convertTo(thresh, src.depth());
		}

		// Prepare output image
		dst.create(src.size(), src.type());
		CV_Assert(src.data != dst.data);  // no inplace processing

		// Apply thresholding: ( pixel > threshold ) ? foreground : background
		Mat mask;
		switch (type)
		{
		case THRESH_BINARY:      // dst = (src > thresh) ? maxval : 0
		case THRESH_BINARY_INV:  // dst = (src > thresh) ? 0 : maxval
			compare(src, thresh, mask, (type == THRESH_BINARY ? CMP_GT : CMP_LE));
			dst.setTo(0);
			dst.setTo(maxValue, mask);
			break;
		case THRESH_TRUNC:       // dst = (src > thresh) ? thresh : src
			compare(src, thresh, mask, CMP_GT);
			src.copyTo(dst);
			thresh.copyTo(dst, mask);
			break;
		case THRESH_TOZERO:      // dst = (src > thresh) ? src : 0
		case THRESH_TOZERO_INV:  // dst = (src > thresh) ? 0 : src
			compare(src, thresh, mask, (type == THRESH_TOZERO ? CMP_GT : CMP_LE));
			dst.setTo(0);
			src.copyTo(dst, mask);
			break;
		default:
			CV_Error( CV_StsBadArg, "Unknown threshold type" );
			break;
		}
	}
	void Binarization::operator()(const Mat &color_img, const vector<vector<CvContour*>> &rst_text_lines, 
		vector<Mat> &binary_text_lines, BinarizationType BT)
	{
		ImgBinarization(color_img, rst_text_lines, binary_text_lines, BT);
	}


	void Binarization::ImgBinarization(const Mat &color_img, const vector<vector<CvContour*>> &rst_text_lines, 
		vector<Mat> &binary_text_lines, BinarizationType BT)
	{
		for (size_t k = 0; k < rst_text_lines.size(); k++)
		{
			// find pixel in a text line (obtain from RST result), compute its binary image
			vector<Point> text_pixels;
			Mat binary_img = Mat::zeros(color_img.size(), CV_8UC1);
			uchar * pBinary_img = binary_img.data;
			for (size_t j = 0; j < rst_text_lines[k].size(); j++)
			{
				CvContour seq = *(rst_text_lines[k][j]);
				for (int u = 0; u < seq.total; u++) 
				{ 
					CvPoint *pos = CV_GET_SEQ_ELEM(CvPoint, &seq, u);
					text_pixels.push_back(Point(pos->x, pos->y));
					int idx = pos->y * color_img.cols + pos->x;
					pBinary_img[idx] = 255;
				}
			}

			// find the expanded boundingbox
			Rect bbox = boundingRect( text_pixels );
			Rect bbox_ex = bbox + Size(10, 10); // distance transform need to pad 5;
			bbox_ex = bbox_ex - Point(5, 5);
			bbox_ex = bbox_ex & Rect(0, 0, color_img.cols - 1, color_img.rows - 1);
			// use the function of clone, otherwise the kmeans clustering result is wrong
			Mat color_roi = color_img(bbox_ex).clone();
			Mat binary_roi = binary_img(bbox_ex).clone();

			// use different method for binarization
			Mat binary_text; // output binary text line image
			switch (BT)
			{
			case BinarizationType::KMEANS_COLOR:
				// use multi-times K-means clustering and refining strategy
				KmeansClusterAndRefine(color_roi, binary_roi, binary_text);
				break;		
			case BinarizationType::KMEANS_COLOR_SSP:
				// use SSP for kmeans centers initialization and image binarization
				KmeansBinarySSP(color_img, binary_img, text_pixels, binary_text);
				break;
			case BinarizationType::OTSU:
				// use otsu method for image binarization
				OTSUBinary(color_roi, binary_roi, binary_text);
				break;
			}	
			// the binary text lines
			binary_text_lines.push_back(binary_text); 
		}
	}


	void Binarization::KmeansClusterAndRefine(const Mat &color_roi, const Mat &binary_roi, Mat &binary_img)
	{
		vector<Point> foreground;
		Mat foreground_img, refine_img;
		// use multi-times kmeans to obtain binary image (denoted as foreground_img)
		int IsUseKmeans_flag = KmeansBinaryMulti(color_roi, binary_roi, foreground, foreground_img);

		// use the original information (stroke and color) to refine the obtained binary image
		if (IsUseKmeans_flag == 0)
			RefineNewCC(color_roi, binary_roi, foreground_img, refine_img);
		else if (IsUseKmeans_flag == 1)
			RefineNewCCWithoutColor(color_roi, binary_roi, foreground_img, refine_img);	

		// remove noise with too small area and remove overlapped rects
		// the final binarization result is denoted as refine_img
		int small_area = 3;
		if (IsUseKmeans_flag == 2)
		{
			small_area = 10;
			foreground_img.copyTo(refine_img);
		}
		RefineNewCCWithGeoFeature(color_roi, binary_roi, refine_img, refine_img, small_area);
		refine_img.copyTo(binary_img);
	}



	void Binarization::OTSUBinary(const Mat& color_roi, const Mat& binary_roi, Mat& textline_img)
	{
		// find the roi 
		Mat gray_roi;
		cvtColor(color_roi, gray_roi, CV_BGR2GRAY);

		// use otsu for image binarization
		Mat foreground1, foreground2;
		threshold(gray_roi, foreground1, 0, 255, CV_THRESH_OTSU);
		foreground2 = foreground1 < 100;

		// compute the foreground image
		uchar * pBinary_roi = binary_roi.data;
		uchar * pForeground1 = foreground1.data;
		uchar * pForeground2 = foreground2.data;
		int count1 = 0, count2 = 0;
		for (int j = 0; j < binary_roi.rows; ++j)
		{
			for (int i = 0; i < binary_roi.cols; ++i)
			{
				int idx = j * binary_roi.cols + i;
				if (pBinary_roi[idx] == 0)
					continue;
				if (pForeground1[idx] > 0)
					count1++;
				else if (pForeground2[idx] > 0)
					count2++;
			}
		}
		if (count1 > count2)
			foreground1.copyTo(textline_img);
		else
			foreground2.copyTo(textline_img);
	}


	void Binarization::BinaryImgFeature(const Mat &color_img, const Mat &binary_img, Scalar &stroke_mean, Scalar &color_mean)
	{
		// for an input binary image, compute mean stroke width and mean color values of the foreground pixels
		Mat dMap;
		distanceTransform(binary_img, dMap, CV_DIST_L1,3);
		Mat skeleton = Mat::zeros(binary_img.size(), CV_8UC1);
		Mat imgguo = binary_img.clone();
		// remove the boundary pixel, otherwise there is errors when compute stroke width for some images
		imgguo.row(0) = 0; imgguo.row(imgguo.rows-1)=0;
		imgguo.col(0) = 0; imgguo.col(imgguo.cols-1)=0;
		GuoHallThinning(imgguo,skeleton);
		Mat mask;
		skeleton.copyTo(mask);
		Scalar stroke_std, color_std;
		// compute stroke width feature
		meanStdDev(dMap, stroke_mean, stroke_std, mask); 
		// compute color feature 
		meanStdDev(color_img, color_mean, color_std, mask);
	}

	int Binarization::KmeansBinaryMulti(const Mat& color_roi, const Mat& binary_roi, 
		vector<Point>& foreground, Mat& foreground_img)
	{	

		uchar * pColor_roi = color_roi.data;
		uchar * pBinary_roi = binary_roi.data;

		// construct cluster data for kmeans clustering
		// cluster_data is in sample_num rows and 3 cols, 3 corresponding to BGR color features
		int sample_num = binary_roi.rows * binary_roi.cols;
		Mat cluster_data = Mat::zeros(sample_num, 3, CV_32FC1);
		float * pCluster_data = (float*) cluster_data.data;
		for (int i = 0; i < color_roi.cols; ++i)
		{
			for (int j = 0; j < color_roi.rows; ++j)
			{
				int idx_color = (j * color_roi.cols + i) * 3;
				for (int u = 0; u < 3; u++)
					pCluster_data[idx_color + u] = pColor_roi[idx_color + u];	
			}
		}

		// compute feature (color+stroke) of the original textline
		// origion_strokeMean and origin_colorMean denote the mean stroke and color values of the original binary image
		Scalar origin_strokeMean, origin_colorMean;
		BinaryImgFeature(color_roi, binary_roi, origin_strokeMean, origin_colorMean);

		// multi-times kmeans
		// new_strokeMean and new_colorMean denote the mean stroke and color values of the foreground binary image generated by Kmeans clustering
		Scalar new_strokeMean, new_colorMean;
		int cluster_num = 2;
		Mat best_label = Mat::zeros(sample_num, 1, CV_32S);
		int * pBest_label = (int*) best_label.data;

		// the feature difference between the orignal binary image obtained from RST and new binary image obtained from Kmeans clustering result
		float origin_feature_dif = 100.0f, origin_color_dif = 100.0f;
		float min_feature_dif = 0.7f, min_color_dif = 0.2f;
		// if origin_feature_dif < min_feature_dif and origin_color_dif < min_color_dif,
		// features between the kmeans result images are close to those of the original image
		// if so, directly break the while

		// if two successive feature_dif (obtained from different K and K+1) is smaller than "termcrit_eps_by_feature_dif", then break while
		float termcrit_eps_by_feature_dif = 0.05f; 
		int max_cluster_num = 5; // maximum iteration steps
		while (cluster_num < max_cluster_num)
		{
			// use k-means method for clustering   
			best_label = Mat::zeros(sample_num, 1, CV_32S);
			Mat centers(cluster_num, cluster_data.cols, cluster_data.type());
			TermCriteria criteria = TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 10, 1.0);
			// "KMEANS_PP_CENTERS" means use the Kmeans++ method for centers initialization 
			kmeans(cluster_data, cluster_num, best_label, criteria, 1, KMEANS_PP_CENTERS , centers);	

			// construct different color layers based on kmeans result
			vector<vector<Point>> pts(cluster_num); // pixels in different binary images of kmeans clustering results
			vector<Mat> binary_imgs(cluster_num); // binary images of kmeans clustering result
			vector<int> count(cluster_num); // compute the overlapped pixels between the kmeans clustering binary images and the original RST binary image
			// the larger the count value, the more possible it is a foreground image
			// extract the K binary kmeans results based on the best_label
			for (int u = 0; u < cluster_num; ++u)
			{
				Mat binary1 = Mat::zeros(color_roi.size(), CV_8UC1);
				uchar * pBinary1 = binary1.data;
				vector<Point> oneLayer;
				int num = 0;
				for (int j = 0; j < binary_roi.rows; j++)
				{
					for (int i = 0; i < binary_roi.cols; i++)
					{
						int idx = j*binary_roi.cols + i;					
						if (pBest_label[idx] == u)
						{
							pBinary1[idx] = 255;	
							oneLayer.push_back(Point(i, j));

							if (pBinary_roi[idx] > 0)
								num++;
						}					
					}
				}
				binary_imgs[u] = binary1;
				pts[u] = oneLayer;
				count[u] = num;
			}

			// find the foregound image among all kmeans result
			// max overlap clustering result with the roi
			int max_overlap_pixel = count[0];
			int idx_max = 0;
			for (int i = 1; i < cluster_num; i++)
			{
				if (count[i] > max_overlap_pixel )
				{
					idx_max = i;
					max_overlap_pixel = count[i];
				}					      
			}		 
			Mat distMap;
			binary_imgs[idx_max].copyTo(distMap);

			// compute feature (color+stroke) of the new binary image
			BinaryImgFeature(color_roi, distMap, new_strokeMean, new_colorMean);
			float stroke_dif = (float)abs(new_strokeMean[0] - origin_strokeMean[0]);
			float color_dif = (float)(abs(new_colorMean[0] - origin_colorMean[0]) + 
				abs(new_colorMean[1] - origin_colorMean[1]) +
				abs(new_colorMean[2] - origin_colorMean[2]))/150;
			// the feature difference between the kmeans binary image and the original RST binary image
			float current_feature_dif = stroke_dif + color_dif;

			// decide weather k is suitable or not
			// if the feature difference between the kmeans binary image and the RST binary image is too small
			// directly treat the current kmeans binary image as the final binarization result and break multi-times kmeans clustering
			if (current_feature_dif < min_feature_dif && color_dif < min_color_dif)
			{
				origin_feature_dif = current_feature_dif;
				binary_imgs[idx_max].copyTo(foreground_img);
				foreground = pts[idx_max];	
				break;
			}		
			// if feature difference between k and k+1 is large, set k = k + 1 and continue k-means clustering
			if (origin_feature_dif - current_feature_dif > termcrit_eps_by_feature_dif)
			{
				origin_color_dif = origin_feature_dif;
				origin_feature_dif = current_feature_dif;
				cluster_num++;
				binary_imgs[idx_max].copyTo(foreground_img);
				foreground = pts[idx_max];					  
			}
			else
				break;	  

		}

		// if K >= 5, perform otsu for binariztion
		// selecting binarization result with small feature distance between kmeans and otsu results
		float max_feature_dif = 0.1f;
		// use the flag to decide the binary image is obtained from 
		// kmeans clustering (flag = 0)
		// ostu binarization method (flag = 1)
		// the reverse hole image (flag = 2) 
		int flag = 0; 
		if (cluster_num == max_cluster_num || origin_feature_dif > max_feature_dif)
		{
			Mat foreground_otsu;
			OTSUBinary(color_roi, binary_roi, foreground_otsu);
			// compute feature difference between the otsu binary image and RST binary image
			Mat distMap = foreground_otsu.clone();
			BinaryImgFeature(color_roi, distMap, new_strokeMean, new_colorMean);
			float stroke_dif = (float)abs(new_strokeMean[0] - origin_strokeMean[0]);
			float color_dif = (float)(abs(new_colorMean[0] - origin_colorMean[0]) + 
				abs(new_colorMean[1] - origin_colorMean[1]) +
				abs(new_colorMean[2] - origin_colorMean[2]))/150;
			float otsu_feature_dif = stroke_dif + color_dif;
			// compare the otsu feature difference and the kmeans feature difference
			// select binay image with small feature difference with the RST binary image
			if (otsu_feature_dif < origin_feature_dif)
			{ 
				foreground_otsu.copyTo(foreground_img);
				flag = 1;
			}
		}
		// decide the foreground_img is reversed or not
		// if there are too many holes or the hole area is larger than the foreground image
		// the treat the hole_img as foreground_img
		Mat hole_img;
		if (GetHoleImg(foreground_img, hole_img))
		{
			flag = 2;
			foreground_img = hole_img;
		}
		return flag;				
	}


	void Binarization::KmeansBinarySSP(const Mat& img, const Mat& binary_img, 
		const vector<Point>& text_pixels, Mat& foreground_img)
	{
		vector<Point> foreground;
		// find the expanded boundingbox
		Rect bbox = boundingRect( text_pixels );
		Rect bbox_ex = bbox + Size(4, 4);
		bbox_ex = bbox_ex - Point(2, 2);
		bbox_ex = bbox_ex & Rect(0, 0, img.cols-1, img.rows-1);

		// extract SSP pixels
		Mat color_roi, binary_roi;
		img(bbox_ex).copyTo(color_roi);
		binary_img(bbox_ex).copyTo(binary_roi);
		vector<vector<Point>> cluster_pixel;
		SSP(color_roi, binary_roi, cluster_pixel);

#ifdef _DEBUG
		Mat seed_mask = Mat::zeros(img.size(), CV_8UC1);
		uchar * pSeed_mask = seed_mask.data;
		for (size_t i = 0; i < cluster_pixel.size(); i++)
		{
			for (size_t j = 0; j < cluster_pixel[i].size(); j++)
			{
				int idx = (cluster_pixel[i][j].y + bbox_ex.y) * img.cols + 
					cluster_pixel[i][j].x + bbox_ex.x;
				if (i == 0) // text pixel
					pSeed_mask[idx] = 255;
				else if (i == 1) // probable text pixel
					pSeed_mask[idx] = 180;
				else if (i == 3) // probable background pixel
					pSeed_mask[idx] = 100;
			}
		}
#endif

		// use k-means method for clustering 
		// use ssp for k-means center initialization
		uchar * pColor_roi = color_roi.data;
		uchar * pBinary_roi = binary_roi.data;
		int sample_num = bbox_ex.width * bbox_ex.height;
		Mat cluster_data = Mat::zeros(sample_num, 3, CV_32FC1);
		float * pCluster_data = (float*) cluster_data.data;
		Mat best_label = Mat::zeros(sample_num, 1, CV_32S);
		int * pBest_label = (int*) best_label.data;
		int cluster_num = 2;
		Mat centers(cluster_num, 1, cluster_data.type());
		for (size_t i = 0; i < cluster_pixel.size(); i++)
		{
			for (size_t j = 0; j < cluster_pixel[i].size(); j++)
			{
				//int idx = cluster_pixel[i][j].y * bbox_ex.width + cluster_pixel[i][j].x;
				int idx = cluster_pixel[i][j].y * bbox_ex.width + cluster_pixel[i][j].x;
				int idx_color = idx * 3;
				for (size_t u = 0; u < 3; u++)
				{
					pCluster_data[idx_color + u] = pColor_roi[idx_color + u];
					if (i == 0 || i == 1)
						pBest_label[idx] = 0;
					else if (i == 2 || i == 3)
						pBest_label[idx] =1;	
				}
			}
		}
		TermCriteria criteria = TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 10, 1.0);
		kmeans(cluster_data, cluster_num, best_label, criteria, 1, KMEANS_USE_INITIAL_LABELS , centers);	
		vector<vector<Point>> layers(cluster_num);
		vector<int> layer_pixel_num(cluster_num);
		vector<Mat> layer_img(cluster_num, Mat::zeros(img.size(), CV_8UC1));
		for (int j = 0; j < bbox_ex.height; j++)
		{
			for (int i = 0; i < bbox_ex.width; i++)
			{
				int idx = j*bbox_ex.width + i;
				layers[(int)pBest_label[idx]].push_back(Point(i + bbox_ex.x, j + bbox_ex.y));
				if ((int)pBinary_roi[idx] > 0)
					layer_pixel_num[(int)pBest_label[idx]]++;
			}
		}
		if (layer_pixel_num[0] > layer_pixel_num[1])
		{
			foreground = layers[0];
		}	
		else
		{
			foreground = layers[1];
		}	

		foreground_img = Mat::zeros(binary_img.size(), CV_8UC1);
		uchar * pForeground_img = foreground_img.data;
		for (size_t i = 0; i < foreground.size(); i++)
		{			
			int idx = foreground[i].y * img.cols + foreground[i].x;
			pForeground_img[idx] = 255;
		}
		foreground_img = foreground_img(bbox_ex).clone();
	}


	void Binarization::RefineNewCC(const Mat& color_roi, const Mat& binary_roi, 
		const Mat& foreground_img, Mat& refine_img)
	{	
		// find the new CCs in the binary image
		vector<vector<Point>> newCCs;
		vector<float> percent; // indicate the overlap of newCCs with the origin text line image
		vector<Rect> CC_rect;
		vector<Mat> CC_img;
		CCExtraction(foreground_img, binary_roi, newCCs,  percent, CC_rect, CC_img);

		vector<Point> new_pixels;
		vector<int> CC_idx, Candi_idx;
		vector<Scalar> CC_color(newCCs.size(), Scalar(0));
		Scalar std_color;
		for (int i = 0; i < (int)newCCs.size(); i++)
		{
#ifdef _DEBUG
			Mat oneCC = CC_img[i].clone();
#endif
			// color feature
			meanStdDev(color_roi, CC_color[i] , std_color, CC_img[i]);          	

			// directly insert CCs larger than 30 pixels and with more than 70% number of pixel in the original image
			// the directly inserted CCs are treated as the truly CCs for feature comparing
			if (percent[i] > 0.7 && newCCs[i].size()>30)				    
			{
				new_pixels.insert(new_pixels.end(),newCCs[i].begin(), newCCs[i].end());
				CC_idx.push_back(i);
			}
			else
				Candi_idx.push_back(i);  // the other new CCs should be verified 
		}
		Scalar origin_color, origin_std;
		if (CC_idx.size() == 0)
		{	
			meanStdDev(color_roi, origin_color , origin_std, binary_roi);   
		}

		if (Candi_idx.size() > 0)
		{
			// compute origin CC features
			float mean_stroke, origin_stroke_std;
			FeatureExtract(binary_roi, mean_stroke, origin_stroke_std);
			float min_stroke = mean_stroke * 0.4f,
				max_stroke = mean_stroke * 1.6f;
			float min_area = mean_stroke * mean_stroke;
			float max_width = binary_roi.rows * 2.0f;

			// find new satisfying CCs
			float newCC_stroke, newCC_std;
			int theta = 110;
			int revise_stroke = 2, revise_size = 15;
			float noConsiderStroke = 1.2f;
			for (int i = 0; i < (int)Candi_idx.size(); i++)
			{
				// remove CC with few pixels
				int idx = Candi_idx[i];
				if (newCCs[idx].size() < min_area || CC_rect[idx].width > max_width)
					continue;

				// compute stroke width of newCC
				FeatureExtract(CC_img[idx], newCC_stroke, newCC_std);
				if ((int)newCC_stroke <= revise_stroke || (int)newCCs[idx].size() < revise_size)
					theta = 180;

				// compare CC width and stroke feature
				if ( newCC_stroke > min_stroke
					&& newCC_stroke < max_stroke)
				{
					if (newCC_stroke < noConsiderStroke)
						new_pixels.insert(new_pixels.end(),
						newCCs[idx].begin(), newCCs[idx].end());
					else
					{
						// compare color difference between newCC and each already inserted CC
						// as long as color differece between newCC and one of the already inserted CCs less than theta
						// insert the newCC
						if (CC_idx.size() > 0)
						{
							for (size_t j = 0; j < CC_idx.size(); ++j)
							{
								Scalar color_dif = CC_color[idx] - CC_color[CC_idx[j]];
								if (abs(color_dif[0]) + abs(color_dif[1]) + abs(color_dif[2]) < theta)
								{
									new_pixels.insert(new_pixels.end(),
										newCCs[idx].begin(), newCCs[idx].end());
									break;
								}
							}    
						}
						else
						{
							Scalar color_dif = CC_color[idx] - origin_color;
							if (abs(color_dif[0]) + abs(color_dif[1]) + abs(color_dif[2]) < theta)
							{
								new_pixels.insert(new_pixels.end(), newCCs[idx].begin(), newCCs[idx].end()); 
							}
						}
					}
				}				
			}
		}

		// if there is no satisfying CC, insert the original detected text pixels
		if (new_pixels.size() > 0)
		{
			refine_img = Mat::zeros(color_roi.size(), CV_8UC1);
			uchar * pRefine_img = refine_img.data;
			for (size_t i = 0; i < new_pixels.size(); i++)
			{			
				int idx = new_pixels[i].y * color_roi.cols + new_pixels[i].x;
				pRefine_img[idx] = 255;
			}
		}
		else
			binary_roi.copyTo(refine_img);
	}


	void Binarization::RefineNewCCWithoutColor(const Mat& color_roi, const Mat& binary_roi, 
		const Mat& foreground_img, Mat& refine_img)
	{	
		// find the new CCs in the binary image
		vector<vector<Point>> newCCs;
		vector<float> percent; // indicate the overlap of newCCs with the origin text line image
		vector<Rect> CC_rect;
		vector<Mat> CC_img;
		CCExtraction(foreground_img, binary_roi, newCCs,  percent, CC_rect, CC_img);

		vector<Point> new_pixels;
		vector<int> CC_idx, Candi_idx;
		vector<Scalar> CC_color(newCCs.size(), Scalar(0));
		Scalar std_color;
		float min_percent = 0.7f, min_area = 30.0f;
		for (int i = 0; i < (int)newCCs.size(); i++)
		{
			// Mat oneCC = CC_img[i].clone();
			// directly insert CCs larger than 30 pixels and with more than 70% number of pixel in the original image
			// the directly inserted CCs are treated as the truly CCs for feature comparing
			if (percent[i] > min_percent && newCCs[i].size() > min_area)				    
			{
				new_pixels.insert(new_pixels.end(),newCCs[i].begin(), newCCs[i].end());
				CC_idx.push_back(i);
			}
			else
				Candi_idx.push_back(i);  // the other new CCs should be verified 
		}

		if (Candi_idx.size() > 0)
		{
			// compute origin CC features
			float mean_stroke, origin_stroke_std;
			FeatureExtract(binary_roi, mean_stroke, origin_stroke_std);
			float min_stroke = mean_stroke * 0.4f,
				max_stroke = mean_stroke * 1.5f;
			float min_area = mean_stroke * mean_stroke;
			float max_width = binary_roi.rows * 2.0f;

			// find new satisfying CCs
			float newCC_stroke, newCC_std;
			for (size_t i = 0; i < Candi_idx.size(); i++)
			{
				// remove CC with few pixels
				int idx = Candi_idx[i];
				if (newCCs[idx].size() < min_area || CC_rect[idx].width > max_width)
					continue;

				// compute stroke width of newCC
				FeatureExtract(CC_img[idx], newCC_stroke, newCC_std);
				// compare CC width and stroke feature
				if ( newCC_stroke > min_stroke
					&& newCC_stroke < max_stroke)
				{
					new_pixels.insert(new_pixels.end(), newCCs[idx].begin(), newCCs[idx].end());							
				}				
			}
		}

		// if there is no satisfying CC, insert the original detected text pixels
		if (new_pixels.size() > 0)
		{
			refine_img = Mat::zeros(color_roi.size(), CV_8UC1);
			uchar * pRefine_img = refine_img.data;
			for (size_t i = 0; i < new_pixels.size(); i++)
			{			
				int idx = new_pixels[i].y * color_roi.cols + new_pixels[i].x;
				pRefine_img[idx] = 255;
			}
		}
		else
			binary_roi.copyTo(refine_img);
	}


	void Binarization::RefineNewCCWithGeoFeature(const Mat& color_roi, const Mat& binary_roi, 
		const Mat& foreground_img, Mat& refine_img, int small_area)
	{	
		// find the new CCs in the binary image
		vector<vector<Point>> newCCs;
		vector<float> percent; // indicate the overlap of newCCs with the origin text line image
		vector<Rect> CC_rect;
		vector<Mat> CC_img;
		CCExtraction(foreground_img, binary_roi, newCCs,  percent, CC_rect, CC_img);

		refine_img = foreground_img.clone();
		vector<bool> isErased(newCCs.size(), false);

		// remove small area
		for (size_t i = 0; i < newCCs.size(); ++i)
		{
			if ((int)newCCs[i].size() < small_area ) //|| boundingRect(newCCs[i]).height < binary_roi.rows * 0.3
			{
				isErased[i] = true;
			}
		}

		// remove inside rect
		for (size_t i = 0; i < newCCs.size(); ++i)
		{
			if (isErased[i])
				continue;

			Rect rectA = boundingRect(newCCs[i]);
			for (size_t j = i + 1; j < newCCs.size(); ++j)
			{
				if (isErased[j])
					continue;

				Rect rectB = boundingRect(newCCs[j]);
				int min_idx = rectA.area() < rectB.area() ? (int)i : (int)j;
				//int max_idx = rectA.area() >= rectB.area() ? i : j;
				Rect inter_rect = rectA & rectB;
				if (inter_rect.area() > boundingRect(newCCs[min_idx]).area() * 0.8)
					isErased[min_idx] = true;
			}
		}

		// construct refine_img
		for (size_t i = 0; i < newCCs.size(); ++i)
		{				
			if (isErased[i])
			{
				for (size_t j = 0; j < newCCs[i].size(); ++j)
				{
					refine_img.at<uchar>(newCCs[i][j]) = 0;
				}
			}
		}
	}

	void Binarization::SSP(const Mat& color_img, const Mat& binary_img, vector<vector<Point>>& cluster_pixel)
	{
		const uchar * pBinary_img = binary_img.data;
		const uchar * pColor_img = color_img.data;

		// image distance transform for SSP calculation
		Mat DMap = Mat::zeros(binary_img.size(), CV_32FC1);
		distanceTransform(binary_img, DMap, CV_DIST_L1,3);	    
		float * pDMap = (float*) DMap.data;

		// find true text_pixel (SSP)
		vector<Point> text_pixel, probable_text_pixel,
			back_pixel, probable_back_pixel;
		Mat flag_mask = Mat::zeros(binary_img.size(), CV_32S);
		int * pFlag_mask = (int*) flag_mask.data;
		Scalar mean_color=Scalar(0);	
		int num = 0;
		for (int j = 0; j< binary_img.rows; j++)
		{
			for (int i = 0; i < binary_img.cols; i++)
			{
				int idx = j * binary_img.cols + i;
				if ((int)pBinary_img[idx] > 0)
				{
					num++;
					int count = 0;
					for (int u = -1; u <= 1; u++)
					{
						for (int v = -1; v <= 1; v++)
						{
							if (j + u < 0 || j + u >= binary_img.rows ||
								i + v < 0 || i + v >= binary_img.cols )
								continue;

							int idx1 = (j + u) * binary_img.cols + i + v;
							if ((int)pBinary_img[idx1]>0 && 
								((pDMap[idx] > pDMap[idx1])||abs(pDMap[idx]-pDMap[idx1])<0.00001))
								count ++;
						}  
					}

					// find ssp points ,NMS 3*3 neighbor
					const int neighborNum = 9;
					if (count == neighborNum)
					{
						pFlag_mask[idx] = 1; 
						text_pixel.push_back(Point(i, j));
						int idx_color = idx * 3;
						for (int u = 0; u < 3; u++)
							mean_color[u] += pColor_img[idx_color+u];
					}
				}
			}
		}
		float iSSP_num = 1.0f / text_pixel.size();
		for (int u = 0; u < 3; u++)
			mean_color[u] = mean_color[u] * iSSP_num;

		// compute mean color difference and max color difference between
		// pixels in the binary image and the SSP pixels
		Scalar mean_color_dif = Scalar(0);
		Scalar max_color_dif = Scalar(0);
		for (int j = 0; j < binary_img.rows; j++)
		{
			for (int i = 0; i < binary_img.cols; i++)
			{
				int idx = j * binary_img.cols + i;
				if ((int)pBinary_img[idx] > 0 && (int)pFlag_mask[idx] == 0)
				{
					int idx_color = idx * 3;
					for (int u = 0; u < 3; u++)
					{
						float dif_color = (float)abs(pColor_img[idx_color + u] - mean_color[u]);
						mean_color_dif[u] += dif_color;
						if (dif_color > max_color_dif[u])
							max_color_dif[u] = dif_color;
					}
					pFlag_mask[idx] = 1;
					probable_text_pixel.push_back(Point(i, j));
				}
			} 
		}
		float iprobable_text_num = 1.0f / probable_text_pixel.size();
		for ( int u = 0; u < 3; u++ )
			mean_color_dif[u] = mean_color_dif[u] * iprobable_text_num;

		// if pixel with color difference from SSP smaller than mean_dif_color
		// it is a true text pixel
		// if pixel with color differnece between mean_dif_color and max_dif_color
		// it is a probable text pixel
		// if pixel with color differnece lager than max_dif_color
		// it is a background pixel
		for (int j = 0; j < binary_img.rows; j++)
		{
			for (int i = 0; i < binary_img.cols; i++)
			{
				int idx = j * binary_img.cols + i;
				if (pFlag_mask[idx] == 0)
				{
					int idx_color = idx * 3;
					if ( pColor_img[idx_color] <= mean_color_dif[0] &&
						pColor_img[idx_color + 1] <= mean_color_dif[1] &&
						pColor_img[idx_color + 2] <= mean_color_dif[2])
					{
						text_pixel.push_back(Point(i, j));
						pFlag_mask[idx] = 1;
					}	  
					else if (pColor_img[idx_color] <= max_color_dif[0] &&
						pColor_img[idx_color + 1] <= max_color_dif[1] &&
						pColor_img[idx_color + 2] <= max_color_dif[2])
					{
						probable_text_pixel.push_back(Point(i, j));
						pFlag_mask[idx] = 1;
					}
					else
					{
						probable_back_pixel.push_back(Point(i, j));
						pFlag_mask[idx] = 1;
					}
				}
			} 
		}        
		cluster_pixel.push_back(text_pixel);
		cluster_pixel.push_back(probable_text_pixel);
		cluster_pixel.push_back(back_pixel);
		cluster_pixel.push_back(probable_back_pixel);
	}



	void Binarization::CCExtraction(const Mat& binary_img, const Mat& origin_bimg, 
		vector<vector<Point>>& CCs, vector<float>& percent, 
		vector<Rect>& textrects, vector<Mat>& CC_img)
	{
		// use BFS algorithm to extract CCs in a binary image
		uchar * pImg = binary_img.data;
		uchar * pOrigin_bimg = origin_bimg.data;
		Mat mask = Mat::zeros(binary_img.size(), CV_8UC1);
		uchar*pmask = mask.data;
		Mat one_img = Mat::zeros(binary_img.size(), CV_8UC1);
		uchar * pOne_img = one_img.data;

		Rect bbox, bbox_ex;
		for (int j = 0; j < binary_img.rows; j++)
		{
			for (int i = 0; i < binary_img.cols; i++)
			{	
				int idx = j * binary_img.cols + i;
				if ((int)pImg[idx] == 0 || (int)pmask[idx] != 0)
					continue;

				// find the first pixel in a CC;
				pmask[idx] = 255;
				vector<Point> oneCC;
				oneCC.push_back(Point(i, j));

				int num = 0;
				if (pOrigin_bimg[idx] > 0)
					num++;

				one_img = Mat::zeros(binary_img.size(), CV_8UC1);
				pOne_img[idx] = 255;

				// find the 8 neighbor pixels
				int count = 0;
				while (count < (int)oneCC.size())
				{
					Point pt = oneCC[count++];
					for (int u = -1; u <= 1; u++)
					{
						for (int v = -1; v <= 1; v++)
						{
							int x0 = pt.x + u, y0 = pt.y + v;
							if (x0 < 0 || y0 < 0 || y0 >= binary_img.rows || x0 >= binary_img.cols)
								continue;
							int idx = y0 * binary_img.cols + x0;
							if (pmask[idx] != 0 || pImg[idx] == 0)
								continue;
							pmask[idx] = 255;
							oneCC.push_back(Point(x0, y0));
							pOne_img[idx] = 255;

							if (pOrigin_bimg[idx] > 0)
								num++;
						}
					}
				}
				CCs.push_back(oneCC);
				percent.push_back(num * 1.0f / oneCC.size());

				/*  bbox_ex = boundingRect(oneCC) + Size(7, 7);
				bbox_ex -=  Point(3, 3);
				bbox_ex = bbox_ex & Rect(0, 0, binary_img.cols-1, binary_img.rows - 1);*/
				textrects.push_back(boundingRect(oneCC));
				// Mat roi = one_img(bbox_ex).clone();
				Mat roi = one_img.clone();
				CC_img.push_back(roi);
			}
		}
	}


	//	void Binarization::CCExtraction(const Mat& binary_img, vector<vector<Point>> &CCs, int small_area)
	//{
	//	Mat mask = Mat::zeros(binary_img.size(), CV_8UC1);
	//	for (int i = 0; i < binary_img.cols; ++i)
	//	{
	//		for (int j = 0; j < binary_img.rows; ++j)
	//		{
	//			Point seed = Point(i, j);
	//			if (binary_img.at<uchar>(seed) == 0 || mask.at<uchar>(seed) != 0)
	//				continue;
	//
	//			// find the first pixel in a CC;
	//			mask.at<uchar>(seed) = 255;
	//			vector<Point> oneCC;
	//			oneCC.push_back(seed);
	//
	//			// find the 8 neighbor pixels using hessian gray scale value
	//			int count = 0;
	//			Point neighbor_pt;
	//			while (count < (int)oneCC.size())
	//			{
	//				Point pt = oneCC[count++];
	//				for (int u = -1; u <= 1; ++u)
	//				{
	//					for (int v = -1; v <= 1; ++v)
	//					{
	//						neighbor_pt = Point2i(pt.x + u, pt.y + v);
	//						if (neighbor_pt.x < 0 || neighbor_pt.y < 0
	//							|| neighbor_pt.y >= binary_img.rows
	//							|| neighbor_pt.x >= binary_img.cols
	//							|| mask.at<uchar>(neighbor_pt) != 0
	//							|| binary_img.at<uchar>(neighbor_pt) == 0)
	//							continue;
	//
	//						mask.at<uchar>(neighbor_pt) = 255;
	//						oneCC.push_back(neighbor_pt);
	//					}
	//				}
	//			}
	//			if ((int)oneCC.size() > small_area)
	//			   CCs.push_back(oneCC);
	//		}
	//	}
	//}


	static inline void fillHole(const Mat srcBw, Mat &dstBw)  
	{  
		Size m_Size = srcBw.size();  
		Mat Temp = Mat::zeros(m_Size.height+2,m_Size.width+2,srcBw.type());
		srcBw.copyTo(Temp(Range(1, m_Size.height + 1), Range(1, m_Size.width + 1)));  

		floodFill(Temp, Point(0, 0), Scalar(255));   
		Mat cutImg;
		Temp(Range(1, m_Size.height + 1), Range(1, m_Size.width + 1)).copyTo(cutImg);  

		dstBw = srcBw | (~cutImg);  
	}


	bool Binarization::GetHoleImg(const Mat &binary_img, Mat &hole_img)
	{
		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;
		Mat binary_roi = binary_img.clone();
		findContours(binary_roi, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_NONE);

		int hole_num = 0;
		int CC_num = 0;
		for (size_t i = 0; i < hierarchy.size(); ++i)
		{			
			float contour_area = (float)contourArea(Mat(contours.at(i)));
			if (hierarchy[i][3] < 0 && contour_area > 20) // the CC num
				++CC_num;
			else if (hierarchy[i][3] >= 0 && contour_area > 10) // the hole num
				++hole_num;
		}

		Mat fill_img;
		fillHole(binary_img, fill_img);
		hole_img = (~binary_img) & fill_img;
		int CC_area = countNonZero(binary_img);
		int hole_area = countNonZero(hole_img);

		if (hole_num * 1.0f / CC_num > 2 || hole_area > CC_area)
			return true;
		return false;

		//Mat contour_img;
		//vector<Mat> channels(3, binary_img);
		//merge(channels, contour_img);
		//for (size_t i = 0; i < contours.size(); ++i)
		//{
		//	Scalar color(rand()&255, rand()&255, rand()&255);
		//	vector<vector<Point>> contour_temp;
		//	contour_temp.push_back(contours[i]);
		//	drawContours(contour_img, contour_temp, 0, color, 1, 8);
		//}

	}

	bool Binarization::GuoHallThinning(const Mat1b & img, Mat& skeleton)
	{
		uchar* img_ptr = img.data;
		uchar* skel_ptr = skeleton.data;

		for (int row = 0; row < img.rows; ++row)
		{
			for (int col = 0; col < img.cols; ++col)
			{
				if (*img_ptr)
				{
					int key = row * img.cols + col;
					if ((col > 0 && *img_ptr != img.data[key - 1]) ||
						(col < img.cols-1 && *img_ptr != img.data[key + 1]) ||
						(row > 0 && *img_ptr != img.data[key - img.cols]) ||
						(row < img.rows-1 && *img_ptr != img.data[key + img.cols]))
					{
						*skel_ptr = 255;
					}
					else
					{
						*skel_ptr = 128;
					}
				}
				img_ptr++;
				skel_ptr++;
			}
		}

		int max_iters = 10000;
		int niters = 0;
		bool changed = true;

		/// list of keys to set to 0 at the end of the iteration
		deque<int> cols_to_set;
		deque<int> rows_to_set;

		while (changed && niters < max_iters)
		{
			changed = false;
			for (unsigned short iter = 0; iter < 2; ++iter)
			{
				uchar *skeleton_ptr = skeleton.data;
				rows_to_set.clear();
				cols_to_set.clear();
				// for each point in skeleton, check if it needs to be changed
				for (int row = 0; row < skeleton.rows; ++row)
				{
					for (int col = 0; col < skeleton.cols; ++col)
					{
						if (*skeleton_ptr++ == 255)
						{
							bool p2, p3, p4, p5, p6, p7, p8, p9;
							p2 = (skeleton.data[(row-1) * skeleton.cols + col]) > 0;
							p3 = (skeleton.data[(row-1) * skeleton.cols + col+1]) > 0;
							p4 = (skeleton.data[row     * skeleton.cols + col+1]) > 0;
							p5 = (skeleton.data[(row+1) * skeleton.cols + col+1]) > 0;
							p6 = (skeleton.data[(row+1) * skeleton.cols + col]) > 0;
							p7 = (skeleton.data[(row+1) * skeleton.cols + col-1]) > 0;
							p8 = (skeleton.data[row     * skeleton.cols + col-1]) > 0;
							p9 = (skeleton.data[(row-1) * skeleton.cols + col-1]) > 0;

							int C  = (!p2 & (p3 | p4)) + (!p4 & (p5 | p6)) +
								(!p6 & (p7 | p8)) + (!p8 & (p9 | p2));
							int N1 = (p9 | p2) + (p3 | p4) + (p5 | p6) + (p7 | p8);
							int N2 = (p2 | p3) + (p4 | p5) + (p6 | p7) + (p8 | p9);
							int N  = N1 < N2 ? N1 : N2;
							int m  = iter == 0 ? ((p6 | p7 | !p9) & p8) : ((p2 | p3 | !p5) & p4);

							if ((C == 1) && (N >= 2) && (N <= 3) && (m == 0))
							{
								cols_to_set.push_back(col);
								rows_to_set.push_back(row);
							}
						}
					}
				}

				// set all points in rows_to_set (of skel)
				unsigned int rows_to_set_size = (unsigned int)rows_to_set.size();
				for (unsigned int pt_idx = 0; pt_idx < rows_to_set_size; ++pt_idx)
				{
					if (!changed)
						changed = (skeleton.data[rows_to_set[pt_idx] * skeleton.cols + cols_to_set[pt_idx]]) > 0;

					int key = rows_to_set[pt_idx] * skeleton.cols + cols_to_set[pt_idx];
					skeleton.data[key] = 0;
					if (cols_to_set[pt_idx] > 0 && skeleton.data[key - 1] == 128) // left
						skeleton.data[key - 1] = 255;
					if (cols_to_set[pt_idx] < skeleton.cols-1 && skeleton.data[key + 1] == 128) // right
						skeleton.data[key + 1] = 255;
					if (rows_to_set[pt_idx] > 0 && skeleton.data[key - skeleton.cols] == 128) // up
						skeleton.data[key - skeleton.cols] = 255;
					if (rows_to_set[pt_idx] < skeleton.rows-1 && skeleton.data[key + skeleton.cols] == 128) // down
						skeleton.data[key + skeleton.cols] = 255;
				}

				if ((niters++) >= max_iters) // we have done!
					break;
			}
		}
		skeleton = (skeleton != 0);
		return true;
	}



	void Binarization::FeatureExtract(const Mat& CC_img, float& stroke_mean, float& stroke_std)
	{
		// stroke feature
		Mat CC_roi;
		CC_img.copyTo(CC_roi);
		CC_roi.col(0) = 0;
		CC_roi.col(CC_roi.cols - 1) = 0;
		CC_roi.row(0) = 0;
		CC_roi.row(CC_roi.rows - 1) = 0;
		Scalar strokeMean = 1,strokeStd = 1;
		Mat dMap;
		distanceTransform(CC_roi, dMap, CV_DIST_L1,3);
		Mat skeleton = Mat::zeros(CC_roi.size(), CV_8UC1);
		Mat imgguo =CC_roi.clone();
		GuoHallThinning(imgguo,skeleton);
		Mat mask;
		skeleton.copyTo(mask);
		meanStdDev(dMap, strokeMean, strokeStd, mask); 

		stroke_mean = (float)strokeMean[0];
		stroke_std = (float)strokeStd[0];
	}
}