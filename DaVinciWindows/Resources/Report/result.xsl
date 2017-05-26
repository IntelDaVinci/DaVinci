<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE xsl:stylesheet [ <!ENTITY nbsp "&#160;"> ]>
<xsl:stylesheet version="2.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="html" version="1.0" encoding="UTF-8" indent="yes"/>
	<xsl:template match="/">

        <html>
            <head>
                <title>
					Test Report for - <xsl:value-of select="test-report/test-suite/test-device/deviceinfo/@deviceID"/>
				</title>
				
            	<script>
                    function toggle(id) {
                        e = document.getElementById(id)
                        e.style.display = e.style.display == "none" ? "block" : "none"
                    }
                </script>
				<meta charset='utf-8'></meta>
                <STYLE type="text/css">
                    @import "result.css";
                </STYLE>
            </head>
			
            <body>
                <DIV>
                    <TABLE class="title">
                        <TR>
                            <TD width="40%" align="left">
								<img src="logo.gif">
								</img>
							</TD>
                            <TD width="60%" align="left">
								<h1>
									Test Report for - <xsl:value-of select="test-report/test-suite/test-device/deviceinfo/@deviceID"/>
                                </h1>
                            </TD>
                        </TR>
                    </TABLE>
                </DIV>
				
				
                <img src="banner.png" align="left"></img>

                <br></br>

                <center>
                    <a href="#" onclick="toggle('summary');">Show Device Information</a>
                </center>

                <br></br>

                <DIV id="summary" style="display: none">
                    <TABLE class="summary">
                        <TR>
                            <TH colspan="2">Device Information</TH>
                        </TR>
                        <TR>
                            <TD width="60%">
                                <!-- Device information -->
                                <TABLE>
									<TR>
										<TD class="rowtitle">Device</TD>
									</TR> 
									
                                    <TR>
                                        <TD class="rowtitle">Build Manufacturer</TD>
                                        <TD>
                                            <xsl:value-of select="test-report/test-suite/test-device/deviceinfo/@build_manufacturer"/>
                                        </TD>
                                    </TR>
                                    <TR>
                                        <TD class="rowtitle">Device ID</TD>
                                        <TD>
                                            <xsl:value-of select="test-report/test-suite/test-device/deviceinfo/@deviceID"/>
                                        </TD>
                                    </TR>
                                    <TR>
                                        <TD class="rowtitle">Android Version</TD>
                                        <TD>
                                            <xsl:value-of select="test-report/test-suite/test-device/deviceinfo/@buildVersion"/>
                                        </TD>
                                    </TR>
                                    <TR>
                                        <TD class="rowtitle">Build Type</TD>
                                        <TD>
                                            <xsl:value-of select="test-report/test-suite/test-device/deviceinfo/@build_type"/>
                                        </TD>
                                    </TR>
                                    <TR>
                                        <TD class="rowtitle">Build Fingerprint</TD>
                                        <TD>
                                            <xsl:value-of select="test-report/test-suite/test-device/deviceinfo/@build_fingerprint"/>
                                        </TD>
                                    </TR>
                                    <TR>
                                        <TD class="rowtitle">Resolution</TD>
                                        <TD>
                                            <xsl:value-of select="test-report/test-suite/test-device/deviceinfo/@resolution"/>
                                        </TD>
                                    </TR>
                                    
									<TR>
                                        <TD class="rowtitle">Phone number</TD>
                                        <TD>
                                            <xsl:value-of select="test-report/test-suite/test-device/PhoneSubInfo/@subscriberId"/>
                                        </TD>
                                    </TR>
									
                                    <TR>
                                        <TD class="rowtitle">Ip Address</TD>
                                        <TD>
                                            <xsl:value-of select="test-report/test-suite/test-device/deviceinfo/@ipaddress"/>
                                        </TD>
                                    </TR>
									
                                    <TR>
                                        <TD class="rowtitle">Storage devices</TD>
                                        <TD>
                                            <xsl:value-of select="test-report/test-suite/test-device/deviceinfo/@storage_devices"/>
                                        </TD>
                                    </TR>
									
                                </TABLE>
                            </TD>

                            <TD width="40%">
                                <TABLE>
									<TR>
										<TD class="rowtitle">Host</TD>
									</TR> 								
								
									<TR>
                                        <TD class="rowtitle">Host Name</TD>
                                        <TD>
                                            <xsl:value-of select="test-report/test-suite/test-host/os/@name"/>
                                        </TD>
                                    </TR>
									
                                    <TR>
                                        <TD class="rowtitle">Host OS</TD>
                                        <TD>
											<xsl:value-of select="test-report/test-suite/test-host/os/@version"/>
                                        </TD>
                                    </TR>
									
                                    <TR>
                                        <TD class="rowtitle">Host RAM</TD>
                                        <TD>
                                            <xsl:value-of select="test-report/test-suite/test-host/os/@ram"/>
                                        </TD>
                                    </TR>
									
									<TR>
                                        <TD class="rowtitle">Java Version</TD>
                                        <TD>
                                            <xsl:value-of select="test-report/test-suite/test-host/java/@version"/>
                                        </TD>
                                    </TR>
									
									<TR>
                                        <TD class="rowtitle">DaVinci Version</TD>
                                        <TD>
                                            <xsl:value-of select="test-report/test-suite/test-host/davinci/@version"/>
                                        </TD>
                                    </TR>
									
									<TR>
                                        <TD class="rowtitle">
											<a href="./DaVinci.log.txt">DaVinci Log</a>
                                        </TD>
                                    </TR>
									
                                </TABLE>
                            </TD>
                        </TR>
                    </TABLE>
					
                    <br />
                    <br />
                </DIV>
<!--
                <DIV>
                    <TABLE class="summary">
                        <TR>
                            <TH colspan="2">Test Summary</TH>
                        </TR>
						
                        <TR>
                            <TD class="rowtitle">DaVinci version</TD>
                            <TD>
                                <xsl:value-of select="test-report/test-suite/test-host/DaVinci/@version"/>
                            </TD>
                        </TR>
                        
                        <TR>
                            <TD class="rowtitle">Host Info</TD>
                            <TD>
                                <xsl:value-of select="test-report/test-suite/test-host/@name"/>
                                (<xsl:value-of select="test-report/test-suite/test-host/Os/@name"/> - 
                                  <xsl:value-of select="test-report/test-suite/test-host/Os/@version"/>,
								  <xsl:value-of select="test-report/test-suite/test-host/Os/@ram"/>)
                            </TD>
                        </TR>                        
						
                        <TR>
                            <TD class="rowtitle">Statistic:</TD>
                            <TD>
                                
                            </TD>
                        </TR>
						
                        <TR>
                            <TD class="rowtitle">Tests Passed</TD>
                            <TD>
                                <xsl:value-of select="test-report/test-suite/test-summary/@PASS"/>
                            </TD>
                        </TR>
						
                        <TR>
                            <TD class="rowtitle">Tests Failed</TD>
                            <TD>
                                <xsl:value-of select="test-report/test-suite/test-summary/@FAIL"/>
                            </TD>
                        </TR>
						
                        <TR>
                            <TD class="rowtitle">Tests Warning</TD>
                            <TD>
                                <xsl:value-of select="test-report/test-suite/test-summary/@WARNING"/>
                            </TD>
                        </TR>
						
                        <TR>
                            <TD class="rowtitle">Tests Skipped</TD>
                            <TD>
                                <xsl:value-of select="test-report/test-suite/test-summary/@SKIP"/>
                            </TD>
                        </TR>
                    </TABLE>
                </DIV>
-->
                <!-- High level summary of test execution -->
				
				<xsl:if test="test-report/test-suite/test-result/case">
					<h2 align="center">Test Summary by Subcase</h2>
					<DIV>
						<TABLE class="testsummary">
							<TR>
								<TH>Test Case</TH>
								<TH>Passed</TH>
								<TH>Failed</TH>
								<TH>Warning</TH>
								<TH>Skipped</TH>
								<TH>Total Sub Tests</TH>
							</TR>
							
							<xsl:for-each select="test-report/test-suite/test-result">
								<TR>
									<TD>
										<xsl:variable name="href1"><xsl:value-of select="case/@name"/></xsl:variable>
										<a href="#{$href1}"><xsl:value-of select="case/@name"/></a>
									</TD>
									
									<TD>
										<xsl:value-of select="count(case/subcase[@result = 'PASS'])"/>
									</TD>
									
									<TD>
										<xsl:value-of select="count(case/subcase[@result = 'FAIL'])"/>
									</TD>
									
									<TD>
										<xsl:value-of select="count(case/subcase[@result = 'WARNING'])"/>
									</TD>
									
									<TD>
										<xsl:value-of select="count(case/subcase[@result = 'SKIP'])"/>
									</TD>
									
									<TD>
										<xsl:value-of select="count(case/subcase)"/>(Sub Cases)
									</TD>
								</TR>
							</xsl:for-each>
						</TABLE>
					</DIV>
				</xsl:if>
		<!--		
				<br></br>
				<br></br>
				<br></br>
				<br></br>
				<img src="banner.png" align="left"></img>
				<br></br>
		-->		
                <xsl:call-template name="filteredResultTestReport">
                    <xsl:with-param name="header" select="'Test Failures'" />
                    <xsl:with-param name="resultFilter" select="'fail'" />
                </xsl:call-template>

                <xsl:call-template name="filteredResultTestReport">
                    <xsl:with-param name="header" select="'Test Timeouts'" />
                    <xsl:with-param name="resultFilter" select="'timeout'" />
                </xsl:call-template>
				
				<br></br>
				<br></br>
				<br></br>
				<br></br>
				<img src="banner.png" align="left"></img>
				<br></br>
				
                <h2 align="center">Detailed Test Report</h2>
                <xsl:call-template name="detailedTestReport" />
				
				<xsl:call-template name="criticalPathHead" />
				<xsl:call-template name="smokePathHead" />
            </body>
        </html>
    </xsl:template>

    <xsl:template name="smokePathHead" match="test-result">
		<xsl:variable name="numPath" select="count(test-report/test-suite/test-path/smoke-path)" />
		<xsl:if test="$numPath &gt; 0">
			<br></br>
			<br></br>
			<br></br>
			<br></br>
			<img src="banner.png" align="left"></img>
			<br></br>
			
			<h2 align="center">Critical Test Path
			</h2>
				
			<xsl:call-template name="smokeCriticalPath" />			
		</xsl:if>
	</xsl:template>
    
	<xsl:template name="criticalPathHead" match="test-result">
		<xsl:variable name="numPath" select="count(test-report/test-suite/test-path/path)" />
		<xsl:if test="$numPath &gt; 0">
			<br></br>
			<br></br>
			<br></br>
			<br></br>
			<img src="banner.png" align="left"></img>
			<br></br>
			
			<h2 align="center">Critical Test Path ( Coverage: 
				<xsl:value-of select="test-report/test-suite/test-path/path/@coverage"/> )
			</h2>
				
			<xsl:call-template name="criticalPath" />			
		</xsl:if>
	</xsl:template>
	
    <xsl:template name="filteredResultTestReport">
        <xsl:param name="header" />
        <xsl:param name="resultFilter" />
        <xsl:variable name="numMatching" select="count(test-report/test-suite/test-result/case/subcase[@result=$resultFilter])" />
        <xsl:if test="$numMatching &gt; 0">
            <h2 align="center"><xsl:value-of select="$header" /> (<xsl:value-of select="$numMatching"/>)</h2>
            <xsl:call-template name="detailedTestReport">
                <xsl:with-param name="resultFilter" select="$resultFilter"/>
            </xsl:call-template>
        </xsl:if>
    </xsl:template>

    <xsl:template name="detailedTestReport">
        <xsl:param name="resultFilter" />
        <DIV>
            <xsl:for-each select="test-report/test-suite/test-result">
			
			<xsl:if test="common">
                <xsl:if test="$resultFilter='' or count(common/item[@result=$resultFilter]) &gt; 0">

                    <TABLE class="testdetails">
                        <TR>
                            <TD class="package" colspan="3">
                                <xsl:variable name="href2"><xsl:value-of select="@type"/></xsl:variable>
                                <a name="{$href2}">Common Replayer Result</a>
                            </TD>
                        </TR>

                        <TR>
                            <TH width="30%">Test</TH>
                            <TH width="5%">Result</TH>
                            <TH>Details</TH>
                        </TR>

                        <!-- test common -->
                        <xsl:for-each select="common">

                            <xsl:if test="$resultFilter='' or count(item[@result=$resultFilter]) &gt; 0">
                                <!-- emit a blank row before every test suite name -->
                                <xsl:if test="position()!=1">
                                    <TR>
										<TD class="testcasespacer" colspan="3">
										</TD>
									</TR>
                                </xsl:if>

                                <TR>
                                    <TD class="testcase">
                                        <xsl:for-each select="ancestor::common">
                                            <xsl:if test="position()!=1">.</xsl:if>
                                            <xsl:value-of select="@name"/>
                                        </xsl:for-each>
                                        <xsl:text></xsl:text>
                                        <xsl:value-of select="@name"/> --
										<left>
											<a href="#" onclick="toggle('casemore');">more</a>
										</left>
										<DIV id="casemore" style="display: none">
											<TABLE>
												<TR>
													<TD class="rowtitle"> >>Total Message:  
														<xsl:value-of select="@message"/>
													</TD>
												</TR>
												<TR>
													<TD> 
														Logcat: <xsl:value-of select="@logcat"/>
														<xsl:variable name="href003"><xsl:value-of select="@logcat"/>
														</xsl:variable>
														<a href="{$href003}" target="_blank">  Details-Link</a>														
													</TD>
												</TR>
											</TABLE>
										</DIV>
                                    </TD>
									<TD class="testcase">
										<xsl:value-of select="@totalreasult"/>
									</TD>
									<TD class="testcase">
										<xsl:variable name="logpath"><xsl:value-of select="/test-report/test-suite/test-result/@message"/>
										</xsl:variable>
										<xsl:value-of select="/test-report/test-suite/test-result/@message"/>
										<a href="file:\\{$logpath}">(Open Folder)</a>
										Logcat:
										<a href="./logcat.txt" target="_blank">  Details-Link</a>
										
									</TD>
                                </TR>
                            </xsl:if>

                            <!-- test -->
                            <xsl:for-each select="item">
                                <xsl:if test="$resultFilter='' or $resultFilter=@result">
                                    <TR>
                                        <TD class="testname"> -- <xsl:value-of select="@name"/></TD>
										
										<TD class="pass">
										    <div style="text-align: center; margin-left:auto; margin-right:auto;">
                                                <xsl:value-of select="@result"/>
                                            </div>
                                        </TD>
										<TD class="failuredetails">
										
											<xsl:if test="@name='unmatchobject' and @result='FAIL'">
												<div class="details">
													<TABLE class="testdetails">
														<TR>
															<TH width="19%">Line and OPCODE</TH>
															<TH width="40%">ReferenceImage</TH>
															<TH width="40%">TargetImage</TH>
														</TR>
														<xsl:for-each select="unmatchobject">
															<TR>
																<TD width="19%">
																	<xsl:value-of select="@line"/>:<xsl:value-of select="@opcodename"/>
																</TD>
																
																<TD width="40%">
																	<xsl:variable name="referenceImageUrl">
																		<xsl:value-of select="@referenceurl"/>
																	</xsl:variable>
																	
																	<a href="{$referenceImageUrl}" target="_blank"> 
																		<img src="{$referenceImageUrl}" style="max-width:360; max-height:240;"/> 
																	</a>
																</TD>
																
																<TD width="40%">
																	<xsl:variable name="targetImageUrl">
																		<xsl:value-of select="@targeturl"/>
																	</xsl:variable>
																	
																	<a href="{$targetImageUrl}" target="_blank"> 
																		<img src="{$targetImageUrl}" style="max-width:360; max-height:240;"/> 
																	</a>
																</TD>
															</TR>
														</xsl:for-each>
														
													</TABLE>
												</div>
											</xsl:if>
											
										
											<xsl:if test="@name='unmatchimage' and @result='FAIL' ">
												<div class="details">
													<TABLE>
														<TR>
															<TH width="19%">Line and OPCODE</TH>
															<TH width="40%">ReferenceImage</TH>
															<TH width="40%">TargetImage</TH>
														</TR>
														<xsl:for-each select="unmatchimage">
															<TR>
																<TD>
																	<xsl:value-of select="@line"/>:<xsl:value-of select="@opcodename"/>
																</TD>
																
																<TD>
																	<xsl:variable name="referenceImageUrl">
																		<xsl:value-of select="@referenceurl"/>
																	</xsl:variable>
																	
																	<a href="{$referenceImageUrl}" target="_blank"> 
																		<img src="{$referenceImageUrl}" style="max-width:360; max-height:240;"/> 
																	</a>
																</TD>
																
																<TD>
																	<xsl:variable name="targetImageUrl">
																		<xsl:value-of select="@targeturl"/>
																	</xsl:variable>
																	
																	<a href="{$targetImageUrl}" target="_blank"> 
																		<img src="{$targetImageUrl}" style="max-width:360; max-height:240;"/> 
																	</a>
																</TD>
															</TR>
														</xsl:for-each>
														
													</TABLE>
												</div>
											</xsl:if>
											
											
											<xsl:if test="@name='crashdialog' and @result='FAIL'">
												<div class="details">
													<TABLE>
														<TR>
															<TH width="19%">Line and OPCODE</TH>
															<TH width="40%">Crash dialog image</TH>
														</TR>
														<xsl:for-each select="crashdialog">
															<TR>
																<TD>
																	<xsl:value-of select="@name"/>
																</TD>
																
																<TD>
																	<xsl:variable name="crashdialogimageurl">
																		<xsl:value-of select="@url"/>
																	</xsl:variable>
																	
																	<a href="{$crashdialogimageurl}" target="_blank"> 
																		<img src="{$crashdialogimageurl}" style="max-width:360; max-height:240;"/> 
																	</a>
																</TD>
															</TR>
														</xsl:for-each>
														
													</TABLE>
												</div>
											</xsl:if>
											
											
											<xsl:if test="@name='flickimage' and @result='FAIL'">
												<div class="details">
													<TABLE>
														<TR>
															<TH width="19%">Line and OPCODE</TH>
															<TH width="40%">Flick image</TH>
														</TR>
														<xsl:for-each select="flickimage">
															<TR>
																<TD>
																	<xsl:value-of select="@name"/>
																</TD>
																
																<TD>
																	<xsl:variable name="filckimageurl">
																		<xsl:value-of select="@url"/>
																	</xsl:variable>
																	
																	<a href="{$filckimageurl}" target="_blank"> 
																		<img src="{$filckimageurl}" style="max-width:360; max-height:240;"/> 
																	</a>
																	
																	<!--<a href="{$filckimageurl}" target="_blank">  Details-Link</a>-->
																</TD>
															</TR>
														</xsl:for-each>
														
													</TABLE>
												</div>
											</xsl:if>
											
											<xsl:if test="@name='flickeringvideo' and @result!='PASS'">
												<div class="details">
													<xsl:variable name="flickeringvideopath"><xsl:value-of select="@message"/>
													</xsl:variable>
													<xsl:value-of select="@message"/>
													<a href="file:\\{$flickeringvideopath}">(Open Folder)</a>
												</div>
											</xsl:if>
											
											<xsl:if test="@name='blankpagevideo' and @result!='PASS'">
												<div class="details">
													<xsl:variable name="blankpagevideopath"><xsl:value-of select="@message"/>
													</xsl:variable>
													<xsl:value-of select="@message"/>
													<a href="file:\\{$blankpagevideopath}">(Open Folder)</a>
												</div>
											</xsl:if>
											
											<xsl:if test="@name='audio' and @result='FAIL'">
												<div class="details">
													<xsl:value-of select="@message"/>
												</div>
											</xsl:if>
											<xsl:if test="@name='audio quality' and @result!='PASS'">
													<xsl:variable name="logpath"><xsl:value-of select="/test-report/test-suite/test-result/@message"/>
													</xsl:variable>
													<xsl:value-of select="@message"/>
													<a href="file:\\{$logpath}">(Open Folder)</a>
													<xsl:variable name="href_audio_file"><xsl:value-of select="commoninfo/@url"/></xsl:variable>
													<a href="{$href_audio_file}" target="_blank">  Details-Link</a>
											</xsl:if>
											
											<xsl:if test="@name='RunTime' and @result='FAIL'">
												<div class="details">
													<xsl:value-of select="@message"/>
												</div>
											</xsl:if>
											
											<xsl:if test="@name='Running' and @result='FAIL'">
												<div class="details">
													<xsl:value-of select="@message"/>
												</div>
											</xsl:if>
											
											<xsl:if test="@name='QAgent Connection' and @result='FAIL'">
												<div class="details">
													<xsl:value-of select="@message"/>
												</div>
											</xsl:if>
										</TD>
										
									</TR> <!-- finished with a row -->
                                </xsl:if>
                            </xsl:for-each> <!-- end test -->
                        </xsl:for-each> <!-- end test case -->
                    </TABLE>
                </xsl:if>
			</xsl:if>			
			
			
			<xsl:if test="case">
                <xsl:if test="$resultFilter='' or count(case/subcase[@result=$resultFilter]) &gt; 0">

                    <TABLE class="testdetails">
                        <TR>
                            <TD class="package" colspan="3">
                                <xsl:variable name="href2"><xsl:value-of select="case/@type"/></xsl:variable>
                                <a name="{$href2}">Compatibility Test Package: <xsl:value-of select="case/@type"/></a>
                            </TD>
                        </TR>

                        <TR>
                            <TH width="30%">Test</TH>
                            <TH width="5%">Result</TH>
                            <TH>Details</TH>
                        </TR>

                        <!-- test case -->
                        <xsl:for-each select="case">

                            <xsl:if test="$resultFilter='' or count(subcase[@result=$resultFilter]) &gt; 0">
                                <!-- emit a blank row before every test suite name -->
                                <xsl:if test="position()!=1">
                                    <TR>
										<TD class="testcasespacer" colspan="3">
										</TD>
									</TR>
                                </xsl:if>

                                <TR>
                                    <TD class="testcase">
                                        <xsl:for-each select="ancestor::case">
                                            <xsl:if test="position()!=1">.</xsl:if>
                                            <xsl:value-of select="@name"/>
                                        </xsl:for-each>
                                        <xsl:text></xsl:text>
                                        <xsl:value-of select="@name"/> --
										<left>
											<a href="#" onclick="toggle('casemore1');">more</a>
										</left>
										<DIV id="casemore1" style="display: none">
											<TABLE rules="none" cellspacing="0">
												<TR>
													<TD> Activity:  
														<xsl:value-of select="@activity"/>
													</TD>
												</TR>
												<TR>
													<TD> Package:  
														<xsl:value-of select="@package"/>
													</TD>
												</TR>
												<TR>
													<TD>
														Logcat: <a href="DaVinci.log.txt" target="_blank">  Details-Link</a>
													</TD>
												</TR>
											</TABLE>
										</DIV>
                                    </TD>
									<TD class="testcase">
										<xsl:value-of select="@totalreasult"/>
									</TD>
									<TD class="testcase">
										<xsl:if test="@logcat!='N/A'">
											<xsl:value-of select="@message"/>
											Logcat: <xsl:value-of select="@logcat"/>
											<xsl:variable name="href03"><xsl:value-of select="@logcat"/>
											</xsl:variable>
											<a href="{$href03}" target="_blank">  Details-Link</a>
										</xsl:if>
										<xsl:if test="@logcat='N/A'">
											Logcat: <a href="DaVinci.log.txt" target="_blank">  Details-Link</a>
										</xsl:if>
										<xsl:if test="@name='MPM'">
											<xsl:value-of select="@message"/>
										</xsl:if>
									</TD>
                                </TR>
                            </xsl:if>

                            <!-- test -->
                            <xsl:for-each select="subcase">
                                <xsl:if test="$resultFilter='' or $resultFilter=@result">
                                    <TR>
                                        <TD class="testname"> -- <xsl:value-of select="@name"/></TD>

                                        <!-- test results -->
                                        <xsl:choose>

                                            <xsl:when test="string(@KnownFailure)">
                                                <!-- "PASS" indicates the that test actually passed (results have been inverted already) -->
                                                <xsl:if test="@result='PASS'">
                                                    <TD class="pass">
                                                        <div style="text-align: center; margin-left:auto; margin-right:auto;">
                                                            known problem
                                                        </div>
                                                    </TD>
                                                    
													<TD class="failuredetails">
														<div class="details">
															<xsl:value-of select="details/@message"/>
															<xsl:variable name="href3"><xsl:value-of select="details/@detailmessage"/></xsl:variable>
															<a href="{$href3}" target="_blank">  Details-Link</a>
                                                        </div>
                                                    </TD>
                                                </xsl:if>

                                                <!-- "fail" indicates that a known failure actually passed (results have been inverted already) -->
                                                <xsl:if test="@result='fail'">
                                                    <TD class="failed">
                                                        <div style="text-align: center; margin-left:auto; margin-right:auto;">
                                                            <xsl:value-of select="@result"/>
                                                        </div>
                                                    </TD>
													
													<TD class="failuredetails">
                                                        <div class="details">
                                                            A test that was a known failure actually passed. Please check.
                                                        </div>
													</TD>
                                                </xsl:if>
                                            </xsl:when>

                                            <xsl:otherwise>
                                                <xsl:if test="@result='PASS'">
                                                    <TD class="pass">
                                                        <div style="text-align: center; margin-left:auto; margin-right:auto;">
                                                            <xsl:value-of select="@result"/>
                                                        </div>
                                                    </TD>
													
                                                    <TD class="failuredetails">
                                                        <div class="details">
															<xsl:value-of select="details/@message"/>
															<xsl:variable name="href4"><xsl:value-of select="details/@detailmessage"/></xsl:variable>
															<xsl:if test="contains($href4, 'png')">
																<img src="{$href4}" style="max-width:360; max-height:240;"/>
															</xsl:if>
															<a href="{$href4}" target="_blank">  Details-Link</a>
															<!--
															<ul>
                                                              <xsl:for-each select="Details/ValueArray/Value">
                                                                <li><xsl:value-of select="."/></li>
                                                              </xsl:for-each>
                                                            </ul>
															-->
                                                        </div>
                                                    </TD>
                                                </xsl:if>

                                                <xsl:if test="@result='FAIL'">
                                                    <TD class="failed">
                                                        <div style="text-align: center; margin-left:auto; margin-right:auto;">
                                                            <xsl:value-of select="@result"/>
                                                        </div>
                                                    </TD>
													
                                                    <TD class="failuredetails">
                                                        <div class="details">
															<xsl:value-of select="details/@message"/>
															<xsl:variable name="href5"><xsl:value-of select="details/@detailmessage"/></xsl:variable>
															<xsl:if test="contains($href5, 'png')">
																<img src="{$href5}" style="max-width:360; max-height:240;"/>
															</xsl:if>
															<a href="{$href5}" target="_blank">  Details-Link</a>
                                                        </div>
                                                    </TD>
                                                </xsl:if>
												
												<xsl:if test="@result='WARNING'">
                                                    <TD class="warning">
                                                        <div style="text-align: center; margin-left:auto; margin-right:auto;">
                                                            <xsl:value-of select="@result"/>
                                                        </div>
                                                    </TD>
													
                                                    <TD class="failuredetails">
                                                        <div class="details">
															<xsl:value-of select="details/@message"/>
															<xsl:variable name="href6"><xsl:value-of select="details/@detailmessage"/></xsl:variable>
															<xsl:if test="contains($href6, 'png')">
																<img src="{$href6}" style="max-width:360; max-height:240;"/>
															</xsl:if>
															<a href="{$href6}" target="_blank">  Details-Link</a>
                                                        </div>
                                                    </TD>
                                                </xsl:if>
												
												<xsl:if test="@result='FPS'">
                                                    <TD class="warning">
                                                        <div style="text-align: center; margin-left:auto; margin-right:auto;">
                                                            <xsl:value-of select="@avgfpsvalue"/>
                                                        </div>
                                                    </TD>
													
                                                    <TD class="failuredetails">
														<div class="details">
														<xsl:if test="details/@message!='N/A'">
															<xsl:value-of select="details/@message"/>
															<xsl:if test="details/@detailmessage!='N/A'">
																<xsl:variable name="href7"><xsl:value-of select="details/@detailmessage"/></xsl:variable>
																<xsl:if test="contains($href7, 'png')">
																	<img src="{$href7}" style="max-width:360; max-height:240;"/>
																</xsl:if>
																<a href="{$href7}" target="_blank">  Details-Link</a>
															</xsl:if>
														</xsl:if>
                                                        </div>
                                                    </TD>
                                                </xsl:if>
												
												<xsl:if test="@result='Launch Time'">
                                                    <TD class="warning">
                                                        <div style="text-align: center; margin-left:auto; margin-right:auto;">
                                                            <xsl:value-of select="@launchtime"/>
                                                        </div>
                                                    </TD>
													
                                                    <TD class="failuredetails">
														<div class="details">
															<TABLE>
																<TR>
																	<TH width="33%">Reference Image</TH>
																	<TH width="33%">Target Image</TH>
																	<TH width="33%">Before Target Image</TH>
																</TR>
																
																	<TR>
																		<TD>
																			<xsl:variable name="referenceimageurl">
																				<xsl:value-of select="@referenceimageurl"/>
																			</xsl:variable>
																			
																			<a href="{$referenceimageurl}" target="_blank"> 
																				<img src="{$referenceimageurl}" style="max-width:360; max-height:240;"/> 
																			</a>
																		</TD>
																		
																		<TD>
																			<xsl:variable name="targetimageurl">
																				<xsl:value-of select="@targetimageurl"/>
																			</xsl:variable>
																			
																			<a href="{$targetimageurl}" target="_blank"> 
																				<img src="{$targetimageurl}" style="max-width:360; max-height:240;"/> 
																			</a>
																		</TD>
																																				
																		<TD>
																			<xsl:variable name="beforeimageurl">
																				<xsl:value-of select="@beforeimageurl"/>
																			</xsl:variable>
																			
																			<a href="{$beforeimageurl}" target="_blank"> 
																				<img src="{$beforeimageurl}" style="max-width:360; max-height:240;"/> 
																			</a>
																		</TD>
																	</TR>
															</TABLE>
														</div>
                                                    </TD>
                                                </xsl:if>

                                                <xsl:if test="@result='timeout'">
                                                    <TD class="timeout">
                                                        <div style="text-align: center; margin-left:auto; margin-right:auto;">
                                                            <xsl:value-of select="@result"/>
                                                        </div>
													</TD>
													
													<TD class="failuredetails">
														<div class="details">
															<xsl:value-of select="details/@message"/>
															<xsl:variable name="href6"><xsl:value-of select="details/@detailmessage"/></xsl:variable>
															<a href="{$href6}" target="_blank">  Details-Link</a>
                                                        </div>
                                                    </TD>
                                                </xsl:if>

                                                <xsl:if test="@result='SKIP'">
                                                    <TD class="notExecuted">
                                                        <div style="text-align: center; margin-left:auto; margin-right:auto;">
                                                            <xsl:value-of select="@result"/>
                                                        </div>
                                                    </TD>
													
                                                    <TD class="failuredetails">
														<div class="details">
															<xsl:value-of select="details/@message"/>
															<!--
															<xsl:variable name="href7"><xsl:value-of select="details/@detailmessage"/></xsl:variable>
															<a href="{$href7}" target="_blank">  Details-Link</a>
															-->
                                                        </div>
                                                    </TD>
                                                </xsl:if>
                                            </xsl:otherwise>
                                        </xsl:choose>
                                    </TR> <!-- finished with a row -->
                                </xsl:if>
                            </xsl:for-each> <!-- end test -->
                        </xsl:for-each> <!-- end test case -->
                    </TABLE>
                </xsl:if>
            </xsl:if>
			
			</xsl:for-each> <!-- end test package -->
        </DIV>
    </xsl:template>
	
    <xsl:template name="criticalPath">
        <xsl:param name="path" />
        <DIV>
            <xsl:for-each select="test-report/test-suite/test-path">
				
				<xsl:if test="path">
					<TABLE class="testdetails">
						<TR>
								<TD class="package" colspan="4">
									<xsl:variable name="hrefpath2"><xsl:value-of select="@type"/></xsl:variable>
									<a name="{$hrefpath2}">Critical Path Information</a>
								</TD>
						</TR>

						<TR>
								<TH width="10%">Name</TH>
								<TH width="40%">Reference Image</TH>
								<TH width="40%">Replay Image</TH>
                                <TH width="9%">Match Result</TH>
						</TR>

						<!-- test path -->
						<xsl:for-each select="path">
								<!--<xsl:if test="$resultFilter='' or count(item[@result=$resultFilter]) &gt; 0">
									 emit a blank row before every test suite name -->
							<xsl:if test="position()!=1">
								<TR>
									<TD class="testcasespacer" colspan="4">
									</TD>
								</TR>
							</xsl:if>

							<TR>
								<TD class="testcase" colspan="4">
									<xsl:for-each select="ancestor::path">
										<xsl:if test="position()!=1">.</xsl:if>
										<xsl:value-of select="@name"/>
										</xsl:for-each>
								</TD>
							</TR>

							<!-- test -->
							<xsl:for-each select="item">
								<xsl:if test="@name='criticalpath'">
									<div class="details">													
										<xsl:for-each select="criticalpath">
											<TR>
												<TD width="10%">
													<xsl:value-of select="@line"/>:<xsl:value-of select="@opcodename"/>
												</TD>
																	
												<TD width="40%">
													<xsl:variable name="pathreferenceImageUrl">
														<xsl:value-of select="@referenceurl"/>
													</xsl:variable>
																		
													<a href="{$pathreferenceImageUrl}" target="_blank"> 
														<img src="{$pathreferenceImageUrl}" style="max-width:360; max-height:240;"/> 
													</a>
												</TD>
																	
												<TD width="40%">
													<xsl:variable name="pathtargetImageUrl">
													<xsl:value-of select="@targeturl"/>
													</xsl:variable>
																		
													<a href="{$pathtargetImageUrl}" target="_blank"> 
														<img src="{$pathtargetImageUrl}" style="max-width:360; max-height:240;"/> 
													</a>
												</TD>

                                                <TD width="9%">
                                                    <xsl:value-of select="@matchresult"/>
                                                </TD>
											</TR>
										</xsl:for-each>											
									</div>
								</xsl:if>
							</xsl:for-each> <!-- end test -->							
						</xsl:for-each> <!-- end test case -->
					</TABLE>						
				</xsl:if>				
			</xsl:for-each> <!-- end test package -->
        </DIV>
    </xsl:template>
    
    <xsl:template name="smokeCriticalPath">
        <xsl:param name="smoke-path" />
        <DIV>
            <xsl:for-each select="test-report/test-suite/test-path">
				
				<xsl:if test="smoke-path">
					<TABLE class="testdetails">
						<TR>
								<TD class="package" colspan="3">
									<xsl:variable name="hrefpath2"><xsl:value-of select="@type"/></xsl:variable>
									<a name="{$hrefpath2}">Critical Path Information</a>
								</TD>
						</TR>

						<TR>
                                <TH width="15%">Loop</TH>
								<TH width="15%">TimeStamp</TH>
								<TH width="70%">Image</TH>
						</TR>

						<!-- test path -->
						<xsl:for-each select="smoke-path">
							<!-- test -->
							<xsl:for-each select="item">
								<xsl:if test="@name='smokecriticalpath'">
									<div class="details">													
										<xsl:for-each select="smokecriticalpath">
											<TR>
												<TD width="15%">
													<xsl:value-of select="@loop"/>
												</TD>
												<TD width="15%">
													<xsl:value-of select="@timestamp"/>
												</TD>				
												<TD width="70%">
													<xsl:variable name="imageurl">
														<xsl:value-of select="@imageurl"/>
													</xsl:variable>
																		
													<a href="{$imageurl}" target="_blank"> 
														<img src="{$imageurl}" style="max-width:360; max-height:240;"/> 
													</a>
												</TD>
											</TR>
										</xsl:for-each>											
									</div>
								</xsl:if>
							</xsl:for-each> <!-- end test -->							
						</xsl:for-each> <!-- end test case -->
					</TABLE>						
				</xsl:if>				
			</xsl:for-each> <!-- end test package -->
        </DIV>
    </xsl:template>
	
    <!-- Take a delimited string and insert line breaks after a some number of elements. --> 
    <xsl:template name="formatDelimitedString">
        <xsl:param name="string" />
        <xsl:param name="numTokensPerRow" select="10" />
        <xsl:param name="tokenIndex" select="1" />
        <xsl:if test="$string">
            <!-- Requires the last element to also have a delimiter after it. -->
            <xsl:variable name="token" select="substring-before($string, ';')" />
            <xsl:value-of select="$token" />
            <xsl:text>&#160;</xsl:text>
          
            <xsl:if test="$tokenIndex mod $numTokensPerRow = 0">
                <br />
            </xsl:if>

            <xsl:call-template name="formatDelimitedString">
                <xsl:with-param name="string" select="substring-after($string, ';')" />
                <xsl:with-param name="numTokensPerRow" select="$numTokensPerRow" />
                <xsl:with-param name="tokenIndex" select="$tokenIndex + 1" />
            </xsl:call-template>
        </xsl:if>
    </xsl:template>
</xsl:stylesheet>
