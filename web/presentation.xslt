<?xml version="1.0"?>

<!DOCTYPE xsl:stylesheet [
  <!ENTITY config SYSTEM "config.xml">
  <!ENTITY nbsp "&#160;">
]>

<xsl:stylesheet version="1.1" xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
  <xsl:output method="html" media-type="text/html" indent="yes"/>

  <xsl:param name="page"/>

  <!-- values needed for path extending -->
  <xsl:variable name="config">
    &config;
  </xsl:variable>

  <xsl:template match="/">
    <html>
      <head>
        <title>ARAnyM Home Page - <!--xsl:value-of select="$config/menu/item[@name=$page]/title"/--></title>
      </head>
      <body>
        <table width="100%" border="0" cellspacing="0" cellpadding="2">
          <tr>
            <td colspan="4" align="center"> 
              <h2>ARAnyM - <xsl:value-of select="/document/header"/>
              <hr size="5"/>
            </h2>
            </td>
          </tr>
          <tr><td colspan="4"></td></tr>
          <tr>
            <td valign="top">
              <xsl:apply-templates select="$config/menu" mode="menu"/>
            </td>
            <td>&nbsp;&nbsp;&nbsp;&nbsp;</td>
            <td valign="top" width="100%">
              <xsl:apply-templates/>
            </td>
          </tr>
        </table>
      </body>
    </html>
  </xsl:template>

  <xsl:template match="menu" mode="menu">
    <table>
      <xsl:apply-templates select="section" mode="menu"/>
    </table>
  </xsl:template>

  <xsl:template match="section" mode="menu">
    <tr align="left">
      <th>
        &nbsp;&nbsp;
        <xsl:value-of select="title"/>
      </th>
    </tr>
    <xsl:apply-templates select="item" mode="menu"/>
  </xsl:template>

  <xsl:template match="item" mode="menu">
    <tr>
      <td>
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<a href="{link}"><strong><xsl:value-of select="title"/></strong></a>
      </td>
    </tr>
  </xsl:template>

  <xsl:template match="item">
    <tr>
      <th colspan="2" align="left">
        <strong><xsl:value-of select="question"/></strong>
      </th>
    </tr>
    <tr>
      <td>
        <xsl:value-of select="answer"/>
      </td>
    </tr>
  </xsl:template>

  <xsl:template match="document">
    <table align="left" valign="top">
      <xsl:apply-templates/>
    </table>
  </xsl:template>

  <xsl:template match="document/header"/>

  <xsl:template match="chapter">
    <tr align="left">
      <th colspan="2">
        <xsl:value-of select="title"/>
      </th>
    </tr>
    <tr align="left">
      <td valign="top">
        <blockquote>
          <xsl:apply-templates select="text"/>
        </blockquote><br/>
      </td>
    </tr>
  </xsl:template>

  <xsl:template match="*">
    <xsl:copy>
      <xsl:copy-of select="@*"/>
      <xsl:apply-templates select="* | text()"/>
    </xsl:copy>
  </xsl:template>

</xsl:stylesheet>
