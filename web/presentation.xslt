<?xml version="1.0"?>

<!DOCTYPE xsl:stylesheet [
  <!ENTITY config SYSTEM "config.xml">
]>

<xsl:stylesheet version="1.1" xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>
  <xsl:output method="html" media-type="text/html" indent="yes"/>

  <xsl:param name="page"/>

  <!-- values needed for path extending -->
  <xsl:variable name="config">
    &config;
  </xsl:variable>
  <xsl:template name="nbsp">
    <xsl:text disable-output-escaping="yes">&amp;</xsl:text><xsl:text>nbsp;</xsl:text>
  </xsl:template>

  <xsl:template match="/">
    <html>
      <head>
        <title>ARAnyM Home Page - <!--xsl:value-of select="$config/menu/item[@name=$page]/title"/--></title>
      </head>
      <body>
        <table width="100%" border="0" cellspacing="0" cellpadding="0">
          <tr>
            <td colspan="4" align="center"><xsl:call-template name="nbsp"/> <h1>ARAnyM Homepage</h1> </td>
          </tr>
          <tr>
            <td valign="top">
              <xsl:apply-templates select="$config/menu" mode="menu"/>
            </td>
            <td>
              <xsl:call-template name="nbsp"/>
              <xsl:call-template name="nbsp"/>
              <xsl:call-template name="nbsp"/>
              <xsl:call-template name="nbsp"/>
            </td>
            <td>
              <xsl:apply-templates/>
            </td>
          </tr>
        </table>
      </body>
    </html>
  </xsl:template>

  <xsl:template match="menu" mode="menu">
    <dl>
      <xsl:apply-templates select="section" mode="menu"/>
    </dl>
  </xsl:template>

  <xsl:template match="section" mode="menu">
    <p>
    <dt><strong>
      <xsl:call-template name="nbsp"/>
      <xsl:call-template name="nbsp"/>
      <xsl:call-template name="nbsp"/>
      <xsl:call-template name="nbsp"/>

      <xsl:value-of select="title"/>
    </strong></dt>
    <xsl:apply-templates select="item" mode="menu"/>
    </p>
  </xsl:template>

  <xsl:template match="item" mode="menu">
    <dd><a href="{link}"><strong><xsl:value-of select="title"/></strong></a></dd>
  </xsl:template>

  <xsl:template match="item">
    <p>
      <dt><strong><xsl:value-of select="question"/></strong></dt>
      <dd><xsl:value-of select="answer"/></dd>
    </p>
  </xsl:template>

  <xsl:template match="document">
    <dl>
      <xsl:apply-templates/>
    </dl>
  </xsl:template>

  <xsl:template match="chapter">
    <dt><strong><xsl:value-of select="title"/></strong></dt>
    <dd><xsl:apply-templates select="text"/></dd>
  </xsl:template>

  <xsl:template match="text">
    <xsl:copy-of select="* | text()"/>
  </xsl:template>

  <xsl:template match="*">
    <xsl:copy-of select="."/>
  </xsl:template>

</xsl:stylesheet>
