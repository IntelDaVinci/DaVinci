#include "MSER.hpp"

namespace DaVinci
{
    typedef struct CvLinkedPoint
    {
        struct CvLinkedPoint* prev;
        struct CvLinkedPoint* next;
        CvPoint pt;
    }
    CvLinkedPoint;

    // the history of region grown
    typedef struct CvMSERGrowHistory
    {
        struct CvMSERGrowHistory* shortcut;
        struct CvMSERGrowHistory* child;
        int stable; // when it ever stabled before, record the size
        int val;
        int size;
    }
    CvMSERGrowHistory;

    typedef struct CvMSERConnectedComp
    {
        CvLinkedPoint* head;
        CvLinkedPoint* tail;
        CvMSERGrowHistory* history;
        unsigned long grey_level;
        int size;
        int dvar; // the derivative of last var
        float var; // the current variation (most time is the variation of one-step back)

        CvContour *children;
    }
    CvMSERConnectedComp;


    //// Linear Time MSER claims by using bsf can get performance gain, here is the implementation
    //// however it seems that will not do any good in real world test
    //inline void _bitset(unsigned long * a, unsigned long b)
    //{
    //    *a |= 1<<b;
    //}

    //inline void _bitreset(unsigned long * a, unsigned long b)
    //{
    //    *a &= ~(1<<b);
    //}

    CvMSERParams cvMSERParams( int delta, int minArea, int maxArea, float maxVariation, float minDiversity )
    {
        CvMSERParams params;
        params.delta = delta;
        params.minArea = minArea;
        params.maxArea = maxArea;
        params.maxVariation = maxVariation;
        params.minDiversity = minDiversity;
        params.maxEvolution = 200;
        params.areaThreshold = 1.01;
        params.minMargin = 0.003;
        params.edgeBlurSize = 5;
        return params;
    }

    // clear the connected component in stack
    static void
        cvInitMSERComp( CvMSERConnectedComp* comp )
    {
        comp->size = 0;
        comp->var = 0;
        comp->dvar = 1;
        comp->history = NULL;
        comp->children = NULL;
    }

    // add history of size to a connected component
    static void
        cvMSERNewHistory( CvMSERConnectedComp* comp,
        CvMSERGrowHistory* history )
    {
        history->child = history;
        if ( NULL == comp->history )
        {
            history->shortcut = history;
            history->stable = 0;
        } else {
            comp->history->child = history;
            history->shortcut = comp->history->shortcut;
            history->stable = comp->history->stable;
        }
        history->val = comp->grey_level;
        history->size = comp->size;
        comp->history = history;
    }

    // merging two connected component
    static void
        cvMSERMergeComp( CvMSERConnectedComp* comp1,
        CvMSERConnectedComp* comp2,
        CvMSERConnectedComp* comp,
        CvMSERGrowHistory* history )
    {
        CvLinkedPoint* head;
        CvLinkedPoint* tail;
        comp->grey_level = comp2->grey_level;
        history->child = history;
        // select the winner by size
        if ( comp1->size >= comp2->size )
        {
            if ( NULL == comp1->history )
            {
                history->shortcut = history;
                history->stable = 0;
            } else {
                comp1->history->child = history;
                history->shortcut = comp1->history->shortcut;
                history->stable = comp1->history->stable;
            }
            if ( NULL != comp2->history && comp2->history->stable > history->stable )
                history->stable = comp2->history->stable;
            history->val = comp1->grey_level;
            history->size = comp1->size;
            // put comp1 to history
            comp->var = comp1->var;
            comp->dvar = comp1->dvar;
            if ( comp1->size > 0 && comp2->size > 0 )
            {
                comp1->tail->next = comp2->head;
                comp2->head->prev = comp1->tail;
            }
            head = ( comp1->size > 0 ) ? comp1->head : comp2->head;
            tail = ( comp2->size > 0 ) ? comp2->tail : comp1->tail;
            // always made the newly added in the last of the pixel list (comp1 ... comp2)
        } else {
            if ( NULL == comp2->history )
            {
                history->shortcut = history;
                history->stable = 0;
            } else {
                comp2->history->child = history;
                history->shortcut = comp2->history->shortcut;
                history->stable = comp2->history->stable;
            }
            if ( NULL != comp1->history && comp1->history->stable > history->stable )
                history->stable = comp1->history->stable;
            history->val = comp2->grey_level;
            history->size = comp2->size;
            // put comp2 to history
            comp->var = comp2->var;
            comp->dvar = comp2->dvar;
            if ( comp1->size > 0 && comp2->size > 0 )
            {
                comp2->tail->next = comp1->head;
                comp1->head->prev = comp2->tail;
            }
            head = ( comp2->size > 0 ) ? comp2->head : comp1->head;
            tail = ( comp1->size > 0 ) ? comp1->tail : comp2->tail;
            // always made the newly added in the last of the pixel list (comp2 ... comp1)
        }

        CvContour *child1 = comp1->children;
        CvContour *child2 = comp2->children;

        comp1->children = NULL;
        comp2->children = NULL;

        CvContour *it = child1;
        if (it != NULL) {
            comp->children = child1;
            while (it->h_next)
                it = (CvContour *)it->h_next;
            it->h_next = (CvSeq *)child2;
            if (child2)
                child2->h_prev = (CvSeq *)it;
        } else
            comp->children = child2;

        comp->head = head;
        comp->tail = tail;
        comp->history = history;
        comp->size = comp1->size + comp2->size;
    }

    static float
        cvMSERVariationCalc( CvMSERConnectedComp* comp,
        int delta )
    {
        CvMSERGrowHistory* history = comp->history;
        int val = comp->grey_level;
        if ( NULL != history )
        {
            CvMSERGrowHistory* shortcut = history->shortcut;
            while ( shortcut != shortcut->shortcut && shortcut->val + delta > val )
                shortcut = shortcut->shortcut;
            CvMSERGrowHistory* child = shortcut->child;
            while ( child != child->child && child->val + delta <= val )
            {
                shortcut = child;
                child = child->child;
            }
            // get the position of history where the shortcut->val <= delta+val and shortcut->child->val >= delta+val
            history->shortcut = shortcut;
            return (float)(comp->size-shortcut->size)/(float)shortcut->size;
            // here is a small modification of MSER where cal ||R_{i}-R_{i-delta}||/||R_{i-delta}||
            // in standard MSER, cal ||R_{i+delta}-R_{i-delta}||/||R_{i}||
            // my calculation is simpler and much easier to implement
        }
        return 1.;
    }

    static bool
        cvMSERStableCheck( CvMSERConnectedComp* comp,
        CvMSERParams params )
    {
        // tricky part: it actually check the stablity of one-step back
        if ( comp->history == NULL || comp->history->size <= params.minArea || comp->history->size >= params.maxArea )
            return 0;
        float div = (float)(comp->history->size-comp->history->stable)/(float)comp->history->size;
        float var = cvMSERVariationCalc( comp, params.delta );
        int dvar = ( comp->var < var || (unsigned long)(comp->history->val + 1) < comp->grey_level );
        int stable = ( dvar && !comp->dvar && comp->var < params.maxVariation && div > params.minDiversity );
        comp->var = var;
        comp->dvar = dvar;
        if ( stable )
            comp->history->stable = comp->history->size;
        return stable != 0;
    }

    // add a pixel to the pixel list
    static void
        cvAccumulateMSERComp( CvMSERConnectedComp* comp,
        CvLinkedPoint* point )
    {
        if ( comp->size > 0 )
        {
            point->prev = comp->tail;
            comp->tail->next = point;
            point->next = NULL;
        } else {
            point->prev = NULL;
            point->next = NULL;
            comp->head = point;
        }
        comp->tail = point;
        comp->size++;
    }

    // convert the point set to CvSeq
    static CvContour*
        cvMSERToContour( CvMSERConnectedComp* comp,
        CvMemStorage* storage )
    {
        CvSeq* _contour = cvCreateSeq( CV_SEQ_KIND_GENERIC|CV_32SC2, sizeof(CvContour), sizeof(CvPoint), storage );
        CvContour* contour = (CvContour*)_contour;
        cvSeqPushMulti( _contour, 0, comp->history->size );
        CvLinkedPoint* lpt = comp->head;
        for ( int i = 0; i < comp->history->size; i++ )
        {
            CvPoint* pt = CV_GET_SEQ_ELEM( CvPoint, _contour, i );
            pt->x = lpt->pt.x;
            pt->y = lpt->pt.y;
            lpt = lpt->next;
        }
        cvBoundingRect( contour );
        contour->h_next = contour->h_prev = contour->v_next = contour->v_prev = NULL;
        CvContour *it = comp->children;
        while (it != NULL) {
            it->v_prev = (CvSeq *)contour;
            it = (CvContour *)it->h_next;
        }
        contour->v_next = (CvSeq *)comp->children;
        contour->reserved[0] = int(10000*(comp->var));//add varaition, but multiply  by 10000
        comp->children = contour;

        return contour;
    }


    // to preprocess src image to following format
    // 32-bit image
    // > 0 is available, < 0 is visited
    // 17~19 bits is the direction
    // 8~11 bits is the bucket it falls to (for BitScanForward)
    // 0~8 bits is the color
    static int*
        cvPreprocessMSER_8UC1( CvMat* img,
        int*** heap_cur,
        CvMat* src,
        CvMat* mask )
    {
        //fprintf(stderr,"Entering cvPreprocessMSER_8UC1\n");
        int srccpt = src->step-src->cols;
        int cpt_1 = img->cols-src->cols-1;
        int* imgptr = img->data.i;
        int* startptr;

        int level_size[256];
        for ( int i = 0; i < 256; i++ )
            level_size[i] = 0;

        for ( int i = 0; i < src->cols+2; i++ )
        {
            *imgptr = -1;
            imgptr++;
        }
        imgptr += cpt_1-1;
        uchar* srcptr = src->data.ptr;
        if ( mask )
        {
            startptr = 0;
            uchar* maskptr = mask->data.ptr;
            for ( int i = 0; i < src->rows; i++ )
            {
                *imgptr = -1;
                imgptr++;
                for ( int j = 0; j < src->cols; j++ )
                {
                    if ( *maskptr )
                    {
                        if ( !startptr )
                            startptr = imgptr;
                        *srcptr = 0xff-*srcptr;
                        level_size[*srcptr]++;
                        *imgptr = ((*srcptr>>5)<<8)|(*srcptr);
                    } else {
                        *imgptr = -1;
                    }
                    imgptr++;
                    srcptr++;
                    maskptr++;
                }
                *imgptr = -1;
                imgptr += cpt_1;
                srcptr += srccpt;
                maskptr += srccpt;
            }
        } else {
            startptr = imgptr+img->cols+1;
            for ( int i = 0; i < src->rows; i++ )
            {
                *imgptr = -1;
                imgptr++;
                for ( int j = 0; j < src->cols; j++ )
                {
                    *srcptr = 0xff-*srcptr;
                    level_size[*srcptr]++;
                    *imgptr = ((*srcptr>>5)<<8)|(*srcptr);
                    imgptr++;
                    srcptr++;
                }
                *imgptr = -1;
                imgptr += cpt_1;
                srcptr += srccpt;
            }
        }
        for ( int i = 0; i < src->cols+2; i++ )
        {
            *imgptr = -1;
            imgptr++;
        }

        heap_cur[0][0] = 0;
        for ( int i = 1; i < 256; i++ )
        {
            heap_cur[i] = heap_cur[i-1]+level_size[i-1]+1;
            heap_cur[i][0] = 0;
        }
        return startptr;
    }

    static void
        cvExtractMSER_8UC1_Pass( int* ioptr,
        int* imgptr,
        int*** heap_cur,
        CvLinkedPoint* ptsptr,
        CvMSERGrowHistory* histptr,
        CvMSERConnectedComp* comptr,
        int step,
        int stepmask,
        int stepgap,
        CvMSERParams params,
        int color,
        CvSeq* contours,
        CvMemStorage* storage )
    {
        //fprintf(stderr,"Entering cvExtractMSER_8UC1_Pass\n");
        comptr->grey_level = 256;
        comptr++;
        comptr->grey_level = (*imgptr)&0xff;
        cvInitMSERComp( comptr );
        *imgptr |= 0x80000000;
        heap_cur += (*imgptr)&0xff;
        int dir[] = { 1, step, -1, -step };
        for ( ; ; )
        {
            // take tour of all the 4 directions
            while ( ((*imgptr)&0x70000) < 0x40000 )
            {
                // get the neighbor
                int* imgptr_nbr = imgptr+dir[((*imgptr)&0x70000)>>16];
                if ( *imgptr_nbr >= 0 ) // if the neighbor is not visited yet
                {
                    *imgptr_nbr |= 0x80000000; // mark it as visited
                    if ( ((*imgptr_nbr)&0xff) < ((*imgptr)&0xff) )
                    {
                        // when the value of neighbor smaller than current
                        // push current to boundary heap and make the neighbor to be the current one
                        // create an empty comp
                        (*heap_cur)++;
                        **heap_cur = imgptr;
                        *imgptr += 0x10000;
                        heap_cur += ((*imgptr_nbr)&0xff)-((*imgptr)&0xff);
                        imgptr = imgptr_nbr;
                        comptr++;
                        cvInitMSERComp( comptr );
                        comptr->grey_level = (*imgptr)&0xff;
                        continue;
                    } else {
                        // otherwise, push the neighbor to boundary heap
                        heap_cur[((*imgptr_nbr)&0xff)-((*imgptr)&0xff)]++;
                        *heap_cur[((*imgptr_nbr)&0xff)-((*imgptr)&0xff)] = imgptr_nbr;
                    }
                }
                *imgptr += 0x10000;
            }
            int i = (int)(imgptr-ioptr);
            ptsptr->pt = cvPoint( i&stepmask, i>>stepgap );
            // get the current location
            cvAccumulateMSERComp( comptr, ptsptr );
            ptsptr++;
            // get the next pixel from boundary heap
            if ( **heap_cur )
            {
                imgptr = **heap_cur;
                (*heap_cur)--;
            } else {

                heap_cur++;
                unsigned long pixel_val = 0;
                for ( unsigned long i = ((*imgptr)&0xff)+1; i < 256; i++ )
                {
                    if ( **heap_cur )
                    {
                        pixel_val = i;
                        break;
                    }
                    heap_cur++;
                }
                if ( pixel_val )

                {
                    imgptr = **heap_cur;
                    (*heap_cur)--;
                    if ( pixel_val < comptr[-1].grey_level )
                    {
                        // check the stablity and push a new history, increase the grey level
                        if ( cvMSERStableCheck( comptr, params ) )
                        {
                            CvContour* contour = cvMSERToContour( comptr, storage );
                            contour->color = color;
                            cvSeqPush( contours, &contour );
                        }
                        cvMSERNewHistory( comptr, histptr );
                        comptr[0].grey_level = pixel_val;
                        histptr++;
                    } else {
                        // keep merging top two comp in stack until the grey level >= pixel_val
                        for ( ; ; )
                        {
                            comptr--;
                            cvMSERMergeComp( comptr+1, comptr, comptr, histptr );
                            histptr++;
                            if ( pixel_val <= comptr[0].grey_level )
                                break;
                            if ( pixel_val < comptr[-1].grey_level )
                            {
                                // check the stablity here otherwise it wouldn't be an MSER
                                if ( cvMSERStableCheck( comptr, params ) )
                                {
                                    CvContour* contour = cvMSERToContour( comptr, storage );
                                    contour->color = color;
                                    cvSeqPush( contours, &contour );
                                }
                                cvMSERNewHistory( comptr, histptr );
                                comptr[0].grey_level = pixel_val;
                                histptr++;
                                break;
                            }
                        }
                    }
                } else
                    break;
            }
        }
    }

    static void
        cvExtractMSER_8UC1( CvMat* src,
        CvMat* mask,
        CvSeq* contours,
        CvMemStorage* storage,
        CvMSERParams params,
        int direction)
    {
        //fprintf(stderr,"Entering cvExtractMSER_8UC1 %d\n",direction);
        int step = 8;
        int stepgap = 3;
        while ( step < src->step+2 )
        {
            step <<= 1;
            stepgap++;
        }
        int stepmask = step-1;

        // to speedup the process, make the width to be 2^N
        CvMat* img = cvCreateMat( src->rows+2, step, CV_32SC1 );
        int* ioptr = img->data.i+step+1;
        int* imgptr;

        // pre-allocate boundary heap
        int** heap = (int**)cvAlloc( (src->rows*src->cols+256)*sizeof(heap[0]) );
        int** heap_start[256];
        heap_start[0] = heap;

        // pre-allocate linked point and grow history
        CvLinkedPoint* pts = (CvLinkedPoint*)cvAlloc( src->rows*src->cols*sizeof(pts[0]) );
        CvMSERGrowHistory* history = (CvMSERGrowHistory*)cvAlloc( src->rows*src->cols*sizeof(history[0]) );
        CvMSERConnectedComp comp[257];

        imgptr = cvPreprocessMSER_8UC1( img, heap_start, src, mask );
        if (direction <=0)
        {
            // darker to brighter (MSER-)
            //fprintf(stderr,"darker to brighter (MSER-)");
            cvExtractMSER_8UC1_Pass( ioptr, imgptr, heap_start, pts, history, comp, step, stepmask, stepgap, params, -1, contours, storage );
        }

        // Clear the unstable components
        for(int i = 0; i < 257; i++) {
            comp[i].children = NULL;
        }

        if (direction >=0)
        {
            //fprintf(stderr,"brighter to darker (MSER+)");
            // brighter to darker (MSER+)
            imgptr = cvPreprocessMSER_8UC1( img, heap_start, src, mask );
            cvExtractMSER_8UC1_Pass( ioptr, imgptr, heap_start, pts, history, comp, step, stepmask, stepgap, params, 1, contours, storage );
        }

        // clean up
        cvFree( &history );
        cvFree( &heap );
        cvFree( &pts );
        cvReleaseMat( &img );
    }
    void
        cvExtractMSER( CvArr* _img,
        CvArr* _mask,
        CvSeq** _contours,
        CvMemStorage* storage,
        CvMSERParams params,
        int direction) //MSER- (direction < 0)  MSER+ (direction > 0) MSER- and MSER+ (direction == 0)
    {

        //fprintf(stderr,"Entering cvExtractMSER\n");

        CvMat srchdr, *src = cvGetMat( _img, &srchdr );
        CvMat maskhdr, *mask = _mask ? cvGetMat( _mask, &maskhdr ) : 0;
        CvSeq* contours = 0;

        CV_Assert(src != 0);
        CV_Assert(CV_MAT_TYPE(src->type) == CV_8UC1 || CV_MAT_TYPE(src->type) == CV_8UC3);
        CV_Assert(mask == 0 || (CV_ARE_SIZES_EQ(src, mask) && CV_MAT_TYPE(mask->type) == CV_8UC1));
        CV_Assert(storage != 0);

        contours = *_contours = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvSeq*), storage );

        // choose different method for different image type
        // for grey image, it is: Linear Time Maximally Stable Extremal Regions
        // for color image, it is: Maximally Stable Colour Regions for Recognition and Matching
        switch ( CV_MAT_TYPE(src->type) )
        {
        case CV_8UC1:
            cvExtractMSER_8UC1( src, mask, contours, storage, params, direction );
            break;
        case CV_8UC3:
            throw("MSER tree currently do not support 3 channels image");
            //cvExtractMSER_8UC3( src, mask, contours, storage, params );
            break;
        }
    }
}
