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
        <title>ARAnyM - <xsl:value-of select="/document/header"/></title>
      </head>
      <body>
        <table width="100%" border="0" cellspacing="0" cellpadding="2">
          <tr>
            <td valign="top" align="center">
              <h2>
                ARAnyM
              </h2>
            </td>
            <td></td>
            <td colspan="2" align="center"> 
              <h2>
                <xsl:value-of select="/document/header"/>
              </h2>
            </td>
          </tr>
          <tr>
            <td valign="top" colspan="4">
              <hr size="5"/>
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

  <xsl:template match="text">
      <xsl:apply-templates select="* | text()"/>
  </xsl:template>

  <xsl:template match="*">
    <xsl:copy>
      <xsl:copy-of select="@*"/>
      <xsl:apply-templates select="* | text()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:template match="counter">
    <xsl:comment> Begin RealHomepageTools </xsl:comment><script type="text/javascript"><xsl:comment>
    var id=132650
    var ua=navigator.userAgent;if(ua.indexOf('MSIE 3')&gt;0){
    document.write('&lt;img src="http://11.rtcode.com/netpoll/ifree')
    document.write('v3.asp?id='+id+'&amp;js=1&amp;to=0&amp;ref='
    +escape(document.referrer)+'" /&gt;')}
    // </xsl:comment></script><script type="text/javascript"
    src="http://11.rtcode.com/netpoll/ifreev3i.asp?id=132650&amp;to=0">
    </script><script type="text/javascript"><xsl:comment>
    if(ua.indexOf('MSIE ')>0)document.write('&lt;!-' + '-')
    // </xsl:comment></script><noscript><p><img
    src="http://11.rtcode.com/netpoll/ifreev3.asp?id=132650&amp;to=0"
    alt="RealTracker" /></p></noscript>
    <xsl:comment> End RealHomepageTools </xsl:comment>
  </xsl:template>

</xsl:stylesheet>
