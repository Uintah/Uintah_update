<?xml version="1.0"?>

<!--
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in compliance
  with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
  License for the specific language governing rights and limitations under
  the License.
  
  The Original Source Code is SCIRun, released March 12, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1994 
  University of Utah. All Rights Reserved.
-->

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:param name="dir"/>
<xsl:param name="cont"/>

<xsl:template match="table">
  <center><table border="1"><xsl:apply-templates/></table></center>
</xsl:template>

<xsl:template match="entrytbl">
  <table border="0"><xsl:apply-templates/></table>
</xsl:template>

<xsl:template match="tgroup">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="thead">
  <xsl:for-each select="./row">
    <xsl:for-each select="./entry">
      <th><xsl:apply-templates/></th>
    </xsl:for-each>
  </xsl:for-each>
</xsl:template>

<xsl:template match="tbody">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="row">
  <tr><xsl:apply-templates/></tr>
</xsl:template>

<xsl:template match="entry">
  <td><xsl:apply-templates/></td>
</xsl:template>

<xsl:template match="chapter/title">
  <p class="title"><xsl:value-of select="."/></p>
</xsl:template>

<xsl:template match="chapter/subtitle">
  <p class="subtitle"><xsl:value-of select="."/></p>
</xsl:template>

<xsl:template match="sect1/title">
  <xsl:param name="sectn"/>
  <a>
    <xsl:attribute name="name">
      <xsl:value-of select="$sectn"/>
    </xsl:attribute>
    <p class="head"><xsl:value-of select="."/></p>
  </a>
</xsl:template>

<xsl:template match="sect2/title">
  <p class="subhead"><xsl:value-of select="."/></p>
</xsl:template>

<xsl:template match="para">
  <p class="firstpara"><xsl:apply-templates/></p>
</xsl:template>

<xsl:template match="itemizedlist">
  <ul><xsl:apply-templates/></ul>
</xsl:template>

<xsl:template match="orderedlist">
  <ol><xsl:apply-templates/></ol>
</xsl:template>

<xsl:template match="listitem">
  <li><xsl:apply-templates/></li>
</xsl:template>

<xsl:template match="citetitle">
  <b><i><xsl:value-of select="."/></i></b>
</xsl:template>

<xsl:template match="computeroutput">
  <div class="box"><br/>
    <font color="blue">
      <pre class="example"><xsl:apply-templates/></pre>
    </font>
  </div>
</xsl:template>

<xsl:template match="term">
  <b><i><font color="darkgreen"><xsl:value-of select="."/></font></i></b>
</xsl:template>

<xsl:template match="emphasis">
  <b><xsl:apply-templates/></b>
</xsl:template>

<xsl:template match="ulink">
  <a>
    <xsl:attribute name="href">
      <xsl:value-of select="@url"/>
    </xsl:attribute>
    <xsl:apply-templates/>
  </a>
</xsl:template>

<xsl:template match="/book">
<xsl:processing-instruction name="cocoon-format">type="text/html"</xsl:processing-instruction>
<html>

<xsl:variable name="swidk">
<xsl:choose>
  <xsl:when test="$dir=4">../../../..</xsl:when>
  <xsl:when test="$dir=3">../../..</xsl:when>
  <xsl:when test="$dir=2">../..</xsl:when>
  <xsl:when test="$dir=1">..</xsl:when>
</xsl:choose>
</xsl:variable>

<head>
<title><xsl:value-of select="./bookinfo/title" /></title>
<link rel="stylesheet" type="text/css">
<xsl:attribute name="href">
<xsl:value-of select="concat($swidk,'/doc/doc_styles.css')" />
</xsl:attribute>
</link>

</head>
<body>

<!-- *************************************************************** -->
<!-- *************** STANDARD SCI RESEARCH HEADER ****************** -->
<!-- *************************************************************** -->

<center>
<img usemap="#head-links" height="71" width="600" border="0">
<xsl:attribute name="src">
<xsl:value-of select="concat($swidk,'/doc/images/research_menuheader.jpg')" />
</xsl:attribute>
</img>
</center>
<map name="head-links">
	<area href="http://www.sci.utah.edu" shape="rect" coords="7,4,171,33" alt="SCI Home" />
	<area href="http://www.sci.utah.edu/software" shape="rect" coords="490,10,586,32" alt="Software" />

	<area shape="rect" coords="340,10,480,32" alt="Documentation">
        <xsl:attribute name="href">
        <xsl:value-of select="concat($swidk,'/doc/index.html')" />
        </xsl:attribute>
        </area>
 

        <area coords="0,41,156,64" shape="rect" alt="Installation Guide">
        <xsl:attribute name="href">
        <xsl:value-of select="concat($swidk,'/doc/InstallGuide/installguide.xml?cont=0&amp;dir=2')" />
        </xsl:attribute>
        </area>

 
        <area coords="157,41,256,64" shape="rect" alt="User's Guide"> 
        <xsl:attribute name="href">
        <xsl:value-of select="concat($swidk,'/doc/UserGuide/userguide.html')" />
        </xsl:attribute>
        </area>

        <area coords="257,41,397,64" shape="rect" alt="Developer's Guide">
        <xsl:attribute name="href">
        <xsl:value-of select="concat($swidk,'/doc/DeveloperGuide/devguide.html')" />
        </xsl:attribute>
        </area>
 
        <area coords="398,41,535,64" shape="rect" alt="Reference Guide">  
        <xsl:attribute name="href">
        <xsl:value-of select="concat($swidk,'/doc/ReferenceGuide/refguide.html')" />
        </xsl:attribute>
        </area>

        <area coords="536,41,600,64" shape="rect" alt="FAQ">  
        <xsl:attribute name="href">
        <xsl:value-of select="concat($swidk,'/doc/FAQ/faq.html')" />
        </xsl:attribute>
        </area>
</map> 

<!-- *************************************************************** -->
<!-- *************************************************************** -->

<!-- ******* identify *this* source document -->
<xsl:variable name="source">
  <xsl:value-of select="/book/bookinfo/title"/>
</xsl:variable>

<xsl:if test="$cont=0">

<!-- ********** Table of Contents ********* -->

<p class="title">
  <xsl:value-of select="./title" />
</p>

<p class="subtitle">
  <xsl:value-of select="./subtitle" />
</p>

<hr size="1"/>

<xsl:for-each select="./preface">
  <xsl:apply-templates/>
</xsl:for-each>

<hr size="1"/>

<xsl:for-each select="./chapter">
  <xsl:variable name="chapnum"><xsl:number/></xsl:variable>
  <p class="head">
    <a>
      <xsl:attribute name="href">
        <xsl:value-of select="concat($source,'?dir=2&amp;cont=',$chapnum)"/>
      </xsl:attribute>  
      <xsl:value-of select="./title" />
    </a>
  </p>
  <p class="firstpara">
    <xsl:value-of select="./sect1/para" />
  </p>
</xsl:for-each>

</xsl:if>

<xsl:if test="$cont>0">

<!-- *********** Chapters ************ -->

<xsl:for-each select="./chapter">
  <xsl:variable name="chapnum"><xsl:number/></xsl:variable>
  <xsl:if test="$chapnum=$cont">
    <p class="title"><xsl:value-of select="./title"/></p>
  </xsl:if>
</xsl:for-each>

<xsl:for-each select="./chapter">
  <xsl:variable name="chapnum"><xsl:number/></xsl:variable>
  <xsl:if test="$chapnum=$cont">
    <xsl:for-each select="./sect1">
      <xsl:variable name="sectnum"><xsl:number/></xsl:variable>
      <xsl:apply-templates>
        <xsl:with-param name="sectn">
          <xsl:value-of select="$sectnum"/>
        </xsl:with-param>
      </xsl:apply-templates>
    </xsl:for-each>
  </xsl:if>
</xsl:for-each>

</xsl:if>

<!-- ******************************************************************* -->
<!-- *********************** STANDARD SCI FOOTER *********************** -->
<!-- ******************************************************************* -->
<center>
<hr size="1" width="600" />
<font size="-1"><a href="http://www.sci.utah.edu">Scientific Computing and Imaging Institute</a> &#149; <a href="http://www.utah.edu">University of Utah</a> &#149; 
(801) 585-1867</font>
</center>
<!-- ********************* END STANDARD SCI FOOTER ********************* -->
<!-- ******************************************************************* -->

</body>
</html>
</xsl:template>

</xsl:stylesheet>
