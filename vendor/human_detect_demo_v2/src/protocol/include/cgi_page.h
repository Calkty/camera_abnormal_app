#ifndef _INCGI_PAGE_H
#define _INCGI_PAGE_H

#include "cJSON.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
	char xmlVersion[8];
	char xmlEncodingType[16];
	char *pageBuffer;
	int  pageSize;
	char *writeIndex;
	cJSON *json_root;
}CGI_PAGE;

extern int sys_neutral_version(void);

#define ISAPI_XMLNS 				(sys_neutral_version() ? "http://www.std-cgi.com/ver20/XMLSchema" : "http://www.hikvision.com/ver20/XMLSchema") 
#define ISAPI_VERSION "2.0"

char *lastEndTag( char *xmlBuffer , char *tagName);
int cgiPageFreeSize(CGI_PAGE *page);
int cgiAddIntTag( CGI_PAGE *page , char *tagName , int tagValue );
int cgiAddStrTag( CGI_PAGE *page , char *tagName , char* tagValue );
int cgiInsertIntTag(CGI_PAGE *page , char *tagName , int tagValue , char *fatherNodeName );
int cgiInsertStrTag(CGI_PAGE *page , char *tagName , char *tagValue , char *fatherNodeName );
int cgiAddBlockStart( CGI_PAGE *page , char *blockName , char *version , char *xmlNS );
int cgiAddBlockStart_ex( CGI_PAGE *page , char *blockName , char *version , char *xmlNS );
int cgiAddBlockEnd( CGI_PAGE *page , char *blockName );
int cgiInsertBlock(CGI_PAGE *page , char *fatherNodeName ,char *blockName , char *version , char *xmlNS);
int cgiAddBlock( CGI_PAGE *page , char *blockName , char *version , char *xmlNS );
int cgiAddPageDeclaration(CGI_PAGE *page);
int cgiAddAtrributeOptTag_IntValue( CGI_PAGE *page , char *tagName , int tagValue , char *opt_name , char *opt_content );
int cgiAddAtrributeOptTag_strValue( CGI_PAGE *page , char *tagName , char* tagValue , char *opt_name , char *opt_content );
int cgi_addatrribute_opttag_nullvalue( CGI_PAGE *page , char *tagName , char *opt_name , char *opt_content );
int cgi_addatrribute_opttag_defaulttag_nullvalue( CGI_PAGE *page , char *tagName , char *opt_name , char *opt_content, char *default_name , char *default_content);
int cgi_addatrribute_opttag_defaulttag_strvalue( CGI_PAGE *page , char *tagName , char *tagValue , char *opt_name , char *opt_content, char *default_name , char *default_content);
int cgiAddAtrributeMinMaxTag_IntValue( CGI_PAGE *page , char *tagName , int tagValue , int min , int max );
int cgiAddAtrributeMinMaxTag_StrValue( CGI_PAGE *page , char *tagName , char* tagValue , int min , int max );
int cgi_addatrribute_minmaxtag_nullvalue( CGI_PAGE *page , char *tagName , int min , int max );
int cgi_addatrribute_minmaxtag_defaulttag_nullvalue( CGI_PAGE *page , char *tagName , int min , int max, int default_value);
int cgi_addatrribute_minmaxtag_deftag_nullvalue( CGI_PAGE *page , char *tagName , int min , int max, int default_value);
int cgi_addatrribute_minmaxtag_deftag_Intvalue( CGI_PAGE *page , char *tagName , int tagValue, int min , int max, int default_value);

int cgi_addatrribute_floatminmaxtag_nullvalue( CGI_PAGE *page , char *tagName , float min_value , float max_value );
int cgiAddIntTag_withAtrribute( CGI_PAGE *page , char *tagName , int tagValue , const char *fmt, ...);
int getPairValue( char *query, char *name, char *value, int valueLen);
int cgiAddFormat_atrribute_nullvalue(CGI_PAGE *page, char *tagName, const char *fmt, ...);
int cgiAddBlockStart_withAttibute( CGI_PAGE *page , char *blockName , char *version , char *xmlNS , const char *fmt, ...);
int cgiWriteLine( CGI_PAGE *page , char *str );
int cgiAddRootBlockStart( CGI_PAGE *page , char *blockName , char *url , int is_namespace );
int cgiAddCommonBlockStart( CGI_PAGE *page , char *blockName );
int cgiAddStrTag_withAtrribute( CGI_PAGE *page , char *tagName , char* tagValue ,const char *fmt, ...);
int cgiAddFormatStrTag(CGI_PAGE *page, char *tagName, const char *fmt, ...);
int cgiAddStrTag_with_Formatattrib(CGI_PAGE *page, char *tagName, char *tagVal, const char *fmt, ...);
int cgi_addatrribute_floatminmaxtag_defaluttag_nullvalue( CGI_PAGE *page , char *tagName , float min_value , float max_value, float default_value );
int cgiAddUintTag(CGI_PAGE *page , char *tagName , unsigned int tagValue);
int cgiAddAtrributeMinMaxTag_UnsignedIntValue( CGI_PAGE *page , char *tagName , unsigned int tagValue , unsigned int min , unsigned int max );
int cgi_addatrribute_maxtag_nullvalue(CGI_PAGE *page, char *tagName, int max);

int cgi_addatrribute_unsignedint_minmaxtag_nullvalue( CGI_PAGE *page , char *tagName , unsigned int min , unsigned int max );
int cgi_addatrribute_floatminmaxtag_floatvalue( CGI_PAGE *page , char *tagName , float value , float min_value , float max_value );
int cgiAddFloatTag( CGI_PAGE *page , char *tagName , float tagValue );
int get_pair_value(char *query, char *name, char *value, int value_size, char *boundary_sign);
int cgi_addatrribute_minmaxtag_deftag_value( CGI_PAGE *page , char *tagName , int min , int max, int default_value, int value);

int initCGIPage( CGI_PAGE *page 
	, char *xmlVersion 
	, char *xmlEncodingType 
	, char *pageBuffer 
	, int pageSize );

char* cgi_getNextUrlLayer(char *url , char *buf );
#ifdef __cplusplus
}
#endif

#endif

