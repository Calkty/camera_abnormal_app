/**@file 
* @note HangZhou Hikvision System Technology Co., Ltd. All Right Reserved.
* @brief  XML扩展接口
* 
* @author   qishige
* @date     2012-11-17
* @version  4.0
* 
* @note /// 供协议XML解析使用，在ixml库的基础上扩展
* @note History:        
* @note     <author>   <time>    <version >   <desc>
* @note     qishige    2012.11.17  1.0        创建
* @warning  
*/

#ifndef _IXML_EXTEND_H
#define _IXML_EXTEND_H

#ifndef ERROR 
#define ERROR	-1
#endif

#ifndef OK
#define OK	0
#endif

#define INVALID_NUM		(-1)
#define VALID_INT_NUM	1
#define VALID_FLOAT 	2

#define INVALID_RANGE	1
#define VALID_RANGE 	2

#define TRANSFORM_GB_TO_UTF8_RATE 	8 // 本地字符集转为UTF8数组会扩大，暂定位8倍

/*需要直接使用的ixml库函数声明*/
void ixmlNode_getElementsByTagName(IXML_Node *n,const char *tagname,IXML_NodeList **list);
void ixmlNode_getElementsByTagNameNoSibling(IXML_Node *n,const char *tagname,IXML_NodeList **list);

unsigned long ixmlNodeList_length(IXML_NodeList *nList);
IXML_Node *ixmlNodeList_item(IXML_NodeList *nList,unsigned long index);
void ixmlNodeList_free(IXML_NodeList *nList);
void safe_ixmlNodeList_free(IXML_NodeList **nList);

char *xml_getfirsttagval_byname( IN IXML_Node *parentnode, IN const char *item );
IXML_Node * xml_getfirstchildnode_byname( IN IXML_Node * parentnode, IN const char *item );
char *xml_getfirsttagval_byname_with_null( IN IXML_Node *parentnode, IN const char *item , char *tag_exist);

char* xml_getfirstattribval_byname( IN IXML_Node *node, IN const char *attribname );
char* xml_getnode_xmlns( IN IXML_Node *node);
char* xml_getfirsttagval_bynamens( IN IXML_Node *parentnode,
								  IN const char *tagname,
								  IN const char *tagxmlns);
void ixmlnode_getelements_bylocalnamerecursive( IN IXML_Node * n,
									 IN const char *localname,
									 OUT IXML_NodeList ** list );
void ixmlnode_getelements_bylocalname( IN IXML_Node * n,
							IN const char *localname,
							OUT IXML_NodeList ** list );
IXML_Node* xml_getfirstchildnode_bylocalname( IN IXML_Node * parentnode,
							  IN const char *localname );
int xml_getfirst_intvalue_bylocalname(IN IXML_Node *parentnode,
							  IN const char *item, int *int_value);
int xml_getfirst_floatvalue_bylocalname(IN IXML_Node *parentnode,IN const char *item, float *float_value);

char *xml_getfirsttagattribute_bylocalname(IN IXML_Node *parentnode,
							  IN const char *item, IN const char *attribname);
char *xml_getfirsttagval_bylocalname(IN IXML_Node *parentnode,
							  IN const char *item);
char *xml_getdirectchild_value_bylocalname(IN IXML_Node *parentnode,
							  IN const char *item);
IXML_Node *xml_getdirectchildnode_bylocalname(IN IXML_Node *parentnode,
							  IN const char *item);
int xml_getfirsttagattribute_intvalue_bylocalname(IN IXML_Node *parentnode,
							  IN const char *item, IN const char *attribname, IN int *int_value);
int xml_getdirectchild_intvalue_bylocalname(IN IXML_Node *parentnode,
							  IN const char *item, int *int_value);
int check_num_valid(char *str_num);
int check_string_value_right_uuid(char *str_num);
int check_string_value_digit_alpha(char *str_num);

int handle_tagval_range_int(char *tagvalue, int min, int max, int *valid_num);
int handle_tagval_range_float(char *tagvalue, float min, float max, float *valid_num);
int handle_value_range_int(int min, int max, int valid_num);
int handle_value_range_float(float min, float max, float valid_num);

int xml_getfirsttagattribute_floatvalue_bylocalname(IN IXML_Node *parentnode,
							  IN const char *item, IN const char *attribname, IN float *float_value);
int is_url_valid(char *p_url);

BOOL is_tagval_bool(char *p_tag);
int handle_tagval_range_unsigned_int(char *tagvalue, unsigned int min, unsigned int max, unsigned int *valid_num);

#endif
