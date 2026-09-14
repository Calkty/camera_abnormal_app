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

#include "ixml.h"
#include <stdlib.h>
#include <syslog.h>
// #include "ixmlparser.h"
// #include "ixmlmembuf.h"
 
#include "xmlextend.h"
extern int isalpha(int c);

#define SWAP(TYPE,A,B) {TYPE t=A; A=B; B=t;}
extern int isdigit(int c);

/*调试宏*/
#define XML_DBG(arg) printf(arg)
#ifndef MIN
#define MIN(a,b)        ((a) > (b) ? (b) : (a))
#endif

/**@brief		判断是否是合法的整型或浮点型
* @param[in]		char *pValue : 字符串格式的数值表达式
* @param[out]	无
* @return		INVALD_NUM：返回输入的字符串为非法数字
				VALID_INT_NUM：返回输入的字符串为合法整数
				VALID_FLOAT：返回输入的字符串为合法浮点数
*/
int check_num_valid(char *str_num)
{
	int dot_num = 0;
	int decimal_num = 0; //小数位
	char *p_str = NULL;

	if(NULL == str_num || 0 == strlen(str_num))
	{
		XML_DBG( "Invalid input parameter\n");
		return INVALID_NUM;
	}

	p_str = str_num;
	while(' ' == *p_str)
	{
		p_str++;
	}

	/* 允许首字符为正负号或者数字*/
	if(isdigit(*p_str))
	{
		p_str++;
	}
	else if(('-' == *p_str) || ('+' == *p_str))
	{
		/* 首字符为正负号，下一字符为数字*/
		if(0 == isdigit(*(p_str+1)))
		{
			XML_DBG( "Invalid digit\n");
			return INVALID_NUM;
		}
		else
		{
			p_str++;
		}
	}
	else
	{
		XML_DBG( "Invalid num\n");
		return INVALID_NUM;
	}

	while(*p_str != '\0')
	{
		/* 判断是否为空格、数字或者小数点*/
		if(isdigit(*p_str) || ('.' == *p_str) || (' ' == *p_str))
		{
			if((1 == dot_num) && isdigit(*p_str))
			{
				decimal_num++;
			}

			if('.' == *p_str)
			{
				dot_num++;
			}

			/* 如果多于一个小数点，跳出循环*/
			if(dot_num <= 1)
			{
				p_str++;
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	if( *p_str != '\0' || dot_num > 1)
	{
		XML_DBG( "Invalid num\n");
		return INVALID_NUM;
	}
	else
	{
		if(0 == dot_num)
		{
			return VALID_INT_NUM;
		}
		else if(1 == dot_num && decimal_num > 0)
		{
			return VALID_FLOAT;
		}
		else
		{
			return INVALID_NUM;
		}
	}
}
 
 /**@brief		 判断是否是合法的uuid格式;数字+字母+ -
 * @param[in]		 char *pValue : 字符串格式的数值表达式
 * @param[out]	 无
 * @return		 INVALD_NUM：返回输入的字符串为非法数字
				 VALID_INT_NUM：返回输入的字符串为合法整数
 */
 int check_string_value_right_uuid(char *str_num)
 {
	 char *p_str = NULL;
 
	 if(NULL == str_num || 0 == strlen(str_num))
	 {
		 XML_DBG( "Invalid input parameter\n");
		 return INVALID_NUM;
	 }
 
	 p_str = str_num;
  
	 while(*p_str != '\0')
	 {
		 /* 判断是否为空格、数字或者小数点*/
		 if(isdigit(*p_str)
		 	|| isalpha(*p_str)
		 	|| '-'==*p_str)
		 {
		 	p_str++;
		 }
		 else
		 {
			 XML_DBG( "Invalid input parameter\n");
			 return INVALID_NUM;
		 }
	 }
	 return VALID_RANGE;
  }

 /**@brief		 判断字符串是否只有字母和数字
 * @param[in]		 char *pValue : 字符串格式的数值表达式
 * @param[out]	 无
 * @return		 INVALD_NUM：返回输入的字符串为非法数字
				 VALID_INT_NUM：返回输入的字符串为合法整数
 */
 int check_string_value_digit_alpha(char *str_num)
 {
	 char *p_str = NULL;
 
	 if(NULL == str_num || 0 == strlen(str_num))
	 {
		 XML_DBG( "Invalid input parameter\n");
		 return INVALID_NUM;
	 }
 
	 p_str = str_num;
  
	 while(*p_str != '\0')
	 {
		 /* 判断是否为空格、数字或者小数点*/
		 if(isdigit(*p_str)
		 	|| isalpha(*p_str))
		 {
		 	p_str++;
		 }
		 else
		 {
			 XML_DBG( "Invalid input parameter\n");
			 return INVALID_NUM;
		 }
	 }
	 return VALID_RANGE;
  }

 /**@brief		检查标签值是否为整数，并处理越界值
 * @param[in]	char *tagvalue : 字符串格式的数值表达式
 * @param[in]	int min : 标签范围的最小值
 * @param[in]	int max: 标签范围的最大值
 * @param[in]	int def_num: 标签范围的默认值
 * @param[out]	int *valid_num 做了兼容性后的标签值
 * @return		INVALID_NUM：失败，标签不是有效的数字。
				INVALID_RANGE：标签值超出范围 
				VALID_RANGE：标签值没有超出范围
 */
int handle_tagval_range_def_int(char *tagvalue, int min, int max, int *valid_num, int def_num)
{
	int num = 0;

	if(NULL == tagvalue || NULL == valid_num || (min > max) || (def_num < min) || (def_num > max))
	{
		XML_DBG( "Invalid input parameter\n");
		return INVALID_NUM;
	}

	if(VALID_INT_NUM != check_num_valid(tagvalue))
	{
		XML_DBG( "Invalid num\n");
		return INVALID_NUM;
	}
	
	num = atoi(tagvalue);
	if(num < min)
	{
		*valid_num = def_num;
		return INVALID_RANGE;
	}
	else if(num > max)
	{
		*valid_num = def_num;
		return INVALID_RANGE;
	}
	else
	{
		*valid_num = num;
		return VALID_RANGE;
	}
}

 /**@brief		检查标签值是否为整数，并处理越界值
 * @param[in]	char *tagvalue : 字符串格式的数值表达式
 * @param[in]	int min : 标签范围的最小值
 * @param[in]	int max: 标签范围的最大值
 * @param[out]	int *valid_num 做了兼容性后的标签值
 * @return		INVALID_NUM：失败，标签不是有效的数字。
				INVALID_RANGE：标签值超出范围 
				VALID_RANGE：标签值没有超出范围
 */
int handle_tagval_range_int(char *tagvalue, int min, int max, int *valid_num)
{
	int num = 0;

	if(NULL == tagvalue || NULL == valid_num || (min > max))
	{
		XML_DBG( "Invalid input parameter\n");
		return INVALID_NUM;
	}

	if(VALID_INT_NUM != check_num_valid(tagvalue))
	{
		XML_DBG( "Invalid num\n");
		return INVALID_NUM;
	}
	
	num = atoi(tagvalue);
	if(num < min)
	{
		*valid_num = min;
		return INVALID_RANGE;
	}
	else if(num > max)
	{
		*valid_num = max;
		return INVALID_RANGE;
	}
	else
	{
		*valid_num = num;
		return VALID_RANGE;
	}
}

/**@brief	  检查标签值是否为整数/浮点，并处理越界值
* @param[in]   char *tagvalue : 字符串格式的数值表达式
* @param[in]   int min : 标签范围的最小值
* @param[in]   int max: 标签范围的最大值
* @param[out]  int *valid_num 做了兼容性后的标签值
* @return	  INVALID_NUM：失败，标签不是有效的数字。
			  INVALID_RANGE：标签值超出范围 
			  VALID_RANGE：标签值没有超出范围
*/
int handle_tagval_range_float(char *tagvalue, float min, float max, float *valid_num)
{
	float num = 0.0f;

	if(NULL == tagvalue || NULL == valid_num || (min > max))
	{
		XML_DBG( "Invalid input parameter\n");
		return INVALID_NUM;
	}

	if(VALID_FLOAT != check_num_valid(tagvalue) && VALID_INT_NUM != check_num_valid(tagvalue))
	{
		XML_DBG( "Invalid num\n");
		return INVALID_NUM;
	}
  
	num = atof(tagvalue);
	if(num < min)
	{
		*valid_num = min;
		return INVALID_RANGE;
	}
	else if(num > max)
	{
		*valid_num = max;
		return INVALID_RANGE;
	}
	else
	{
		*valid_num = num;
		return VALID_RANGE;
	}
}

 
 
 
 /**@brief	   检查标签值是否为浮点，并处理越界值
 * @param[in]	int min : 标签范围的最小值
 * @param[in]	int max: 标签范围的最大值
 * @param[int]	int *valid_num 做了兼容性后的标签值
 * @return	   INVALID_NUM：失败，标签不是有效的数字。
			   INVALID_RANGE：标签值超出范围 
			   VALID_RANGE：标签值没有超出范围
 */
 int handle_value_range_float(float min, float max, float valid_num)
 {
 
	 if(min > max)
	 {
		 XML_DBG( "Invalid input parameter\n");
		 return INVALID_NUM;
	 }
 
	 if(valid_num < min)
	 {
		 return INVALID_RANGE;
	 }
	 else if(valid_num > max)
	 {
		 return INVALID_RANGE;
	 }
	 else
	 {
		 return VALID_RANGE;
	 }
 }
  /**@brief 	 检查标签值是否符合范围并处理越界值
  * @param[in]	 int min : 标签范围的最小值
  * @param[in]	 int max: 标签范围的最大值
  * @param[out]  int *valid_num 做了兼容性后的标签值
  * @return 	 INVALID_NUM：失败，标签不是有效的数字。
				 INVALID_RANGE：标签值超出范围 
				 VALID_RANGE：标签值没有超出范围
  */
 int handle_value_range_int(int min, int max, int valid_num)
 { 
	 if(min > max)
	 {
		 XML_DBG( "Invalid input parameter\n");
		 return INVALID_NUM;
	 }
	 
	 if(valid_num < min)
	 {
		 valid_num = min;
		 return INVALID_RANGE;
	 }
	 else if(valid_num > max)
	 {
		 valid_num = max;
		 return INVALID_RANGE;
	 }
	 else
	 {
		 return VALID_RANGE;
	 }
 }
 /**@brief 	 检查标签值是否为长整型数，并处理越界值
  * @param[in]	 char *tagvalue : 字符串格式的数值表达式
  * @param[in]	 long min : 标签范围的最小值
  * @param[in]	 long max: 标签范围的最大值
  * @param[out]  long *valid_num 做了兼容性后的标签值
  * @return 	 INVALID_NUM：失败，标签不是有效的数字。
				 INVALID_RANGE：标签值超出范围 
				 VALID_RANGE：标签值没有超出范围
  */
 int handle_tagval_range_unsigned_int(char *tagvalue, unsigned int min, unsigned int max, unsigned int *valid_num)
 {
	 unsigned int num = 0;
 
	 if(NULL == tagvalue || NULL == valid_num || (min > max))
	 {
		 XML_DBG( "Invalid input parameter\n");
		 return INVALID_NUM;
	 }
 
	 if(VALID_INT_NUM != check_num_valid(tagvalue))
	 {
		 XML_DBG( "Invalid num\n");
		 return INVALID_NUM;
	 }
	 
	 num = strtoul(tagvalue,NULL,10);
	 if(num < min)
	 {
		 *valid_num = min;
		 return INVALID_RANGE;
	 }
	 else if(num > max)
	 {
		 *valid_num = max;
		 return INVALID_RANGE;
	 }
	 else
	 {
		 *valid_num = num;
		 return VALID_RANGE;
	 }
 }

/**@brief		获取兼容性的参数
 * @param[in]	int32_t cap_min	设备能力集最小值
 * @param[in]	int32_t cap_max	设备能力集最大值
 * @param[in]	int32_t value 设置的前端值
 * @return		兼容处理后前端参数值
 */
int32_t get_compatible_int_value(int32_t value, int32_t cap_min, int32_t cap_max)
{
	int32_t tmp_value = value;

	if(cap_min > cap_max)
	{
		SWAP(int32_t, cap_min, cap_max);
		XML_DBG( "swap the min and max value\n");
	}

	if(tmp_value > cap_max)
	{
		return cap_max;
	}
	else if(tmp_value < cap_min)
	{
		return cap_min;
	}
	else
	{
		return tmp_value;
	}		
}

void hik_ixmlNode_getElementsByTagName(
	IXML_Node *n,
	const char *tagname,
	IXML_NodeList **list)
{
	*list = ixmlDocument_getElementsByTagName((IXML_Document *)n, tagname);

}

/**@brief	 根据节点名称获取节点值，节点的类型是叶子节点，tag类型的节点	 
* @param[in]  IXML_Node *parentnode : 包含目的节点的父节点，或者是根节点，不必是直接的父节点
* @param[in] const char *item : 节点名称
* @param[out]
* @return	 查找成功，返回节点的值；查找失败，返回NULL.
*/
char *xml_getfirsttagval_byname( IN IXML_Node *parentnode,
							  IN const char *item )
{
	IXML_NodeList *nodelist = NULL;
	IXML_Node *textnode = NULL;
	IXML_Node *tmpnode = NULL;

	char *ret = NULL;

	if( ( NULL == parentnode ) || ( NULL == item ) ) 
	{
		return NULL;
	}

	/*获取所有的同名标签*/
	hik_ixmlNode_getElementsByTagName( ( IXML_Node * ) parentnode, item,
								&nodelist );

	if( NULL != nodelist ) 
	{
		/*列出第一个标签*/
		if( ( tmpnode = ixmlNodeList_item( nodelist, 0 ) ) ) 
		{
			textnode = ixmlNode_getFirstChild( tmpnode );
			if( NULL != textnode)
			{
				ret = textnode->nodeValue;
			}
			else
			{
				/*如果是空标签，会出现这种情况*/
				ret = NULL;
			}
		}
	}

	if( NULL != nodelist )
	{
		safe_ixmlNodeList_free( &nodelist );
	}
	
	return ret;
}

/**@brief	 根据节点名称获取节点值，节点的类型是叶子节点，tag类型的节点	 
* @param[in]  IXML_Node *parentnode : 包含目的节点的父节点，或者是根节点，不必是直接的父节点
* @param[in] const char *item : 节点名称
* @param[out]
* @return	 查找成功，返回节点的值；查找失败，返回NULL.
*/
char *xml_getfirsttagval_byname_with_null( IN IXML_Node *parentnode,
							  IN const char *item , char *tag_exist)
{
	IXML_NodeList *nodelist = NULL;
	IXML_Node *textnode = NULL;
	IXML_Node *tmpnode = NULL;

	char *ret = NULL;

	if( ( NULL == parentnode ) || ( NULL == item ) ) 
	{
		return NULL;
	}
	*tag_exist = FALSE;
	/*获取所有的同名标签*/
	hik_ixmlNode_getElementsByTagName( ( IXML_Node * ) parentnode, item,
								&nodelist );

	if( NULL != nodelist ) 
	{
		/*列出第一个标签*/
		if( ( tmpnode = ixmlNodeList_item( nodelist, 0 ) ) ) 
		{
			textnode = ixmlNode_getFirstChild( tmpnode );
			if( NULL != textnode)
			{
				*tag_exist = TRUE;
				ret = textnode->nodeValue;
			}
			else
			{
				/*如果是空标签，会出现这种情况*/
				*tag_exist = TRUE;
				ret = NULL;
			}
		}
	}

	if( NULL != nodelist )
	{
	 	safe_ixmlNodeList_free( &nodelist );
	}
	
	return ret;
}


/**@brief	 根据节点名称获取节点，此节点可以是中间节点或者是叶子节点
* @param[in]  IXML_Node *parentnode : 包含目的节点的父节点，或者是根节点，不必是直接的父节点
* @param[in] IN const char *item :节点名称
* @param[out]
* @return	 如果查找成功，返回节点指针；如果查找失败，返回NULL
*/
IXML_Node * xml_getfirstchildnode_byname( IN IXML_Node * parentnode,
							  IN const char *item )
{
	IXML_NodeList *nodelist = NULL;
	IXML_Node *tmpnode = NULL;

	if( (NULL == parentnode) || (NULL == item) ) 
	{
		return NULL;
	}

	/*获取所有的同名节点*/
	hik_ixmlNode_getElementsByTagName( ( IXML_Node * ) parentnode, item,
								&nodelist );
	if( nodelist ) 
	{
		/*列出第一个节点*/
		tmpnode = ixmlNodeList_item( nodelist, 0 );
	}

	if( nodelist )
	{
		safe_ixmlNodeList_free( &nodelist );
	}
	
	return tmpnode;
}

/**@brief	 根据属性名称，获取指定节点的属性值
* 示例: <tt:SimpleItem Name="MinCount" Value="5"/>
*       如果输入的节点为SimpleItem，属性名称为Name，函数获取到属性值为MinCount
* @param[in]  IXML_Node *node: 节点指针
* @param[in] const char *attribname :属性名称
* @param[out]
* @return	 如果成功，返回属性值;如果失败返回NULL
*/
char* xml_getfirstattribval_byname( IN IXML_Node *node,
								  IN const char *attribname )
								  
{
	Nodeptr p_attrib = NULL;
	char *p_attribvalue = NULL;
	
	if( NULL == node || NULL == attribname)
	{
		return NULL;
	}

	/*获取第一个属性节点*/
	p_attrib = node->firstAttr;

	/*由第一个属性节点开始，遍历所有的属性名称，进行匹配*/
	while(p_attrib)
	{
		if( 0 == strcmp(p_attrib->nodeName,attribname))
		{
			p_attribvalue = p_attrib->nodeValue;
			break;
		}
		else
		{
			/*取下一个属性*/
			p_attrib = p_attrib->nextSibling;
		}
	}

	return p_attribvalue;
}

/**@brief	 获取一个节点的XML命名空间
* 示例: <IPv4Address xmlns="http://www.onvif.org/ver10/schema">10.1.1.1</IPv4Address>
*       如果输入的节点为IPv4Address，函数获取到该节点的命名空间为http://www.onvif.org/ver10/schema
* @param[in]  IXML_Node *node : 节点指针
* @param[out] 
* @return	 如果命名空间存在，返回命名空间值；如果失败，返回NULL
*/
char* xml_getnode_xmlns( IN IXML_Node *node)
								  
{
	char *xmlns = NULL;
	
	if( NULL == node)
	{
		return NULL;
	}

	xmlns = xml_getfirstattribval_byname(node,"xmlns");
	return xmlns;
}

/**@brief	 根据节点名称和命名空间获取节点值，节点为叶子节点
*示例:<IPv4Address xmlns="http://www.onvif.org/ver10/schema">10.1.1.1</IPv4Address>
*     节点名称为IPv4Address，命名空间为http://www.onvif.org/ver10/schema，从xml树中找到这个节点，把它的值10.1.1.1返回
* @param[in]  IXML_Node *parentnode : 包含目的节点的父节点，或者是根节点，不必是直接的父节点
* @param[in] const char *tagname : 标签的名字
* @param[in] const char *tagxmlns :命名空间
* @param[out]
* @return	查找成功，返回标签值；查找失败，返回NULL
*/
char* xml_getfirsttagval_bynamens( IN IXML_Node *parentnode,
								  IN const char *tagname,
								  IN const char *tagxmlns)
								  
{
	IXML_NodeList *nodelist = NULL;
	IXML_Node *textnode = NULL;
	IXML_Node *tmpnode = NULL;
	int children_num = 0;
	int i = 0;
	char *tagvalue = NULL;

	if(NULL == parentnode || NULL == tagname || NULL == tagxmlns)
	{
		return NULL;
	}

	//获取所有的同名标签
	hik_ixmlNode_getElementsByTagName( ( IXML_Node * ) parentnode, tagname,
								&nodelist );

	if( nodelist ) 
	{
	 	children_num = ixmlNodeList_length(nodelist);

	 	/*进行遍历，找到命名空间一致的标签*/
		for(i =0 ; i < children_num ; i++)
		{
			if( ( tmpnode = ixmlNodeList_item( nodelist, i ) ) ) 
			{
				/*匹配命名空间*/
				if(NULL != xml_getnode_xmlns(tmpnode) && 0 == strcmp(tagxmlns,xml_getnode_xmlns(tmpnode)))
			 	{
				 	textnode = ixmlNode_getFirstChild( tmpnode );
					
				 	if(textnode)
				 	{
				 		tagvalue = textnode->nodeValue;
				 	}
				 	else
				 	{
				 		/*这种情况是:tag名称和命名空间都匹配到了，但是值是空的*/
				 		tagvalue = NULL;
				 	}

				 	break;
				}
				else
				{
					/*如果命名空间没有匹配到，继续找下一个*/
					continue;
				}
			}
			else
			{
				continue;
			}
		}
	}

	if( nodelist )
	{
		safe_ixmlNodeList_free( &nodelist );
	}
	
	return tagvalue;
}

static void ixmlNodeList_init(IXML_NodeList *nList)
{
	assert(nList != NULL);

	memset(nList, 0, sizeof (IXML_NodeList));
}

/**
 * @brief xmlextend.c 封装接口中使用了ixml内部接口，为了兼容性，这里把ixml内部接口实现复制一份
 *
 */
static int ixmlNodeList_addToNodeList(
	IXML_NodeList **nList,
	IXML_Node *add)
{
	IXML_NodeList *traverse = NULL;
	IXML_NodeList *p = NULL;
	IXML_NodeList *newListItem;


	if (add == NULL) {
		return IXML_FAILED;
	}

	if (*nList == NULL) {
		/* nodelist is empty */
		*nList = (IXML_NodeList *)malloc(sizeof (IXML_NodeList));
		if (*nList == NULL) {
			return IXML_INSUFFICIENT_MEMORY;
		}

		ixmlNodeList_init(*nList);
	}

	if ((*nList)->nodeItem == NULL) {
		(*nList)->nodeItem = add;
	} else {
		traverse = *nList;
		while (traverse != NULL) {
			p = traverse;
			traverse = traverse->next;
		}

		newListItem = (IXML_NodeList *)malloc(sizeof (IXML_NodeList));
		if (newListItem == NULL) {
			return IXML_INSUFFICIENT_MEMORY;
		}
		p->next = newListItem;
		newListItem->nodeItem = add;
		newListItem->next = NULL;
	}

	return IXML_SUCCESS;
}
/**@brief	 内部函数: 根据本地名称，获取节点列表
* @param[in]  IXML_Node *n : 包含目的节点的父节点，或者是根节点，不必是直接的父节点
* @param[in]  const char *localname : 节点本地名称
* @param[out] IXML_NodeList ** list : 节点列表指针
* @return	 无
*/
void ixmlnode_getelements_bylocalnamerecursive( IN IXML_Node * n,
									 IN const char *localname,
									 OUT IXML_NodeList ** list )
{
	const char *name = NULL;

	if( NULL == n || NULL == localname || NULL == list)
	{
		return;
	}

	if( n != NULL ) 
	{
		if( ixmlNode_getNodeType( n ) == eELEMENT_NODE ) 
		{
			name = ixmlNode_getLocalName( n );
			if( strcmp( localname, name ) == 0 || strcmp( localname, "*" ) == 0 ) 
			{
				ixmlNodeList_addToNodeList( list, n );
			}
		}

		ixmlnode_getelements_bylocalnamerecursive( ixmlNode_getFirstChild
										 ( n ), localname, list );
		ixmlnode_getelements_bylocalnamerecursive( ixmlNode_getNextSibling
										 ( n ), localname, list );
	}

}

/**@brief	 内部函数: 根据本地名称，获取节点列表
* @param[in]  IXML_Node *n : 包含目的节点的父节点，或者是根节点，不必是直接的父节点
* @param[in]  const char *localname : 节点本地名称
* @param[out] IXML_NodeList ** list : 节点列表指针
* @return	 无
*/
void ixmlnode_getelements_bylocalname( IN IXML_Node * n,
							IN const char *localname,
							OUT IXML_NodeList ** list )
{
	const char *name=NULL;

	
	if( NULL == n || NULL == localname || NULL == list)
	{
		return;
	}

	if( ixmlNode_getNodeType( n ) == eELEMENT_NODE ) 
	{
		name = ixmlNode_getLocalName( n );
		if( strcmp( localname, name ) == 0 || strcmp( localname, "*" ) == 0 ) 
		{
			ixmlNodeList_addToNodeList( list, n );
		}
	}
	
	ixmlnode_getelements_bylocalnamerecursive( ixmlNode_getFirstChild( n ),
										 localname, list );

}

/**@brief	 根据本地名称，获取节点
* @param[in]  IXML_Node * parentnode : 包含目的节点的父节点，或者是根节点，不必是直接的父节点
* @param[in]  const char *localname : 节点本地名称
* @param[out] 无
* @return	 如果成功，返回找到的节点指针；如果失败返回NULL
*/
IXML_Node* xml_getfirstchildnode_bylocalname( IN IXML_Node * parentnode,
							  IN const char *localname )
{
	IXML_NodeList *nodelist = NULL;
	IXML_Node *tmpnode = NULL;

	if( ( parentnode == NULL ) || ( localname == NULL ) ) {
	 return NULL;
	}

	/*获取所有的同名节点*/
	ixmlnode_getelements_bylocalname( ( IXML_Node * ) parentnode, localname,
								&nodelist );
	if( nodelist ) 
	{
		/*列出第一个节点*/
		tmpnode = ixmlNodeList_item( nodelist, 0 );
	}

	if( nodelist )
	{
		safe_ixmlNodeList_free( &nodelist );
	}
	return tmpnode;
}


/**@brief	 根据本地名称，获取标签值
* @param[in]  IXML_Node * parentnode : 包含目的节点的父节点，或者是根节点，不必是直接的父节点
* @param[in]  const char *item : 节点本地名称
* @param[out] 无
* @return	 如果成功，返回标签值；如果失败，返回NULL
*/
char *xml_getfirsttagval_bylocalname(IN IXML_Node *parentnode,
							  IN const char *item)
{
	char *ret = NULL;	
	IXML_Node *node = NULL; //xml的body节点
	IXML_Node *textnode = NULL;

	if(NULL == parentnode || NULL == item)
	{
		return NULL;
	}
	
	node = xml_getfirstchildnode_bylocalname(parentnode, item);
	if(NULL == node)
	{
		return NULL;
	}
	
	textnode = ixmlNode_getFirstChild(node);
	if(NULL != textnode)
	{
		ret = textnode->nodeValue;		
	}
	else
	{
		/*如果是空标签，会出现这种情况*/
		ret = NULL;
	}
	
	return ret;
}

/**@brief	 根据本地名称，获取标签值，本函数按照广度优先，只查找直接子节点
* @param[in]  IXML_Node * parentnode : 包含目的节点的父节点，或者是根节点，不必是直接的父节点
* @param[in]  const char *item : 节点本地名称
* @param[out] 无
* @return	 如果成功，返回标签值；如果失败，返回NULL
*/
char *xml_getdirectchild_value_bylocalname(IN IXML_Node *parentnode,
							  IN const char *item)
{
	IXML_Node *child_node = NULL;
	char *tagvalue = NULL;

	if(NULL == parentnode || NULL == item)
	{
		return NULL;
	}
	
	child_node = parentnode->firstChild;
	
	while((NULL != child_node) && (NULL != child_node->localName))
	{
		if(0 == strcmp(child_node->localName,item))
		{
			/*判断节点的值是否为空*/
			if(NULL == child_node->firstChild)
			{
				tagvalue = NULL;
				break;
			}
			else
			{
				tagvalue = child_node->firstChild->nodeValue;
				break;
			}
		}
		else
		{
			/*如果匹配不到，查找兄弟节点*/
			child_node = child_node->nextSibling;
		}
	}

	return tagvalue;
}


/**@brief	 根据本地名称，获取标签值，本函数按照广度优先，只查找直接子节点
* @param[in]  IXML_Node * parentnode : 包含目的节点的父节点，或者是根节点，不必是直接的父节点
* @param[in]  const char *item : 节点本地名称
* @param[out] 无
* @return	 如果成功，返回标签值；如果失败，返回ERROR
*/
int xml_getdirectchild_intvalue_bylocalname(IN IXML_Node *parentnode,
							  IN const char *item, int *int_value)
{
	IXML_Node *child_node = NULL;
	char *tagvalue = NULL;

	if(NULL == parentnode || NULL == item)
	{
		return ERROR;
	}
	
	child_node = parentnode->firstChild;
	
	while((NULL != child_node) && (NULL != child_node->localName))
	{
		if(0 == strcmp(child_node->localName,item))
		{
			/*判断节点的值是否为空*/
			if(NULL == child_node->firstChild)
			{
				tagvalue = NULL;
				break;
			}
			else
			{
				tagvalue = child_node->firstChild->nodeValue;
				break;
			}
		}
		else
		{
			/*如果匹配不到，查找兄弟节点*/
			child_node = child_node->nextSibling;
		}
	}

	if(INVALID_NUM != check_num_valid(tagvalue))
	{
		*int_value = atoi(tagvalue);
		return OK;
	}

	return ERROR;
}


/**@brief	 根据本地名称，获取标签值，本函数按照广度优先，只查找直接子节点
* @param[in]  IXML_Node * parentnode : 包含目的节点的父节点，或者是根节点，不必是直接的父节点
* @param[in]  const char *item : 节点本地名称
* @param[out] 无
* @return	 如果成功，返回标签值；如果失败，返回NULL
*/
IXML_Node *xml_getdirectchildnode_bylocalname(IN IXML_Node *parentnode,
							  IN const char *item)
{
	IXML_Node *child_node = NULL;
	IXML_Node *tag_node = NULL;

	if(NULL == parentnode || NULL == item)
	{
		return NULL;
	}
	
	child_node = parentnode->firstChild;
	
	while((NULL != child_node) && (NULL != child_node->localName))
	{
		if(0 == strcmp(child_node->localName,item))
		{
			tag_node = child_node;
			break;
		}
		else
		{
			/*如果匹配不到，查找兄弟节点*/
			child_node = child_node->nextSibling;
		}
	}

	return tag_node;
}



/**@brief	 根据本地名称，和属性名称，获取属性值
* @param[in]  IXML_Node * parentnode : 包含目的节点的父节点，或者是根节点，不必是直接的父节点
* @param[in]  const char *item : 节点本地名称
* @param[in]  const char *attribname: 属性名称
* @param[out] 无
* @return	 如果成功，返回属性值；如果失败，返回NULL
*/
char *xml_getfirsttagattribute_bylocalname(IN IXML_Node *parentnode,
							  IN const char *item, IN const char *attribname)
{
	char *ret = NULL;	
	IXML_Node *node = NULL;

	if(NULL == parentnode || NULL == item || NULL == attribname)
	{
		return NULL;
	}

	/* 首先找到该节点 */
	node = xml_getfirstchildnode_bylocalname(parentnode, item);
	if(NULL == node)
	{
		return NULL;
	}

	/* 然后获取属性 */
	ret = xml_getfirstattribval_byname(node, attribname);
	return ret;
}

/**@brief	 根据本地名称，和属性名称，获取属性值
* @param[in]  IXML_Node * parentnode : 包含目的节点的父节点，或者是根节点，不必是直接的父节点
* @param[in]  const char *item : 节点本地名称
* @param[in]  const char *attribname: 属性名称
* @param[out] 无
* @return	 如果成功，返回属性值；如果失败，返回ERROR
*/
int xml_getfirsttagattribute_intvalue_bylocalname(IN IXML_Node *parentnode,
							  IN const char *item, IN const char *attribname, IN int *int_value)
{
	char *ret = NULL;	
	IXML_Node *node = NULL;

	if(NULL == parentnode || NULL == item || NULL == attribname || NULL == int_value)
	{
		return ERROR;
	}

	/* 首先找到该节点 */
	node = xml_getfirstchildnode_bylocalname(parentnode, item);
	if(NULL == node)
	{
		return ERROR;
	}

	/* 然后获取属性 */
	ret = xml_getfirstattribval_byname(node, attribname);
	if(INVALID_NUM != check_num_valid(ret))
	{
		*int_value = atoi(ret);
		return OK;
	}
	
	return ERROR;
}

/**@brief	 根据本地名称，和属性名称，获取属性值
* @param[in]  IXML_Node * parentnode : 包含目的节点的父节点，或者是根节点，不必是直接的父节点
* @param[in]  const char *item : 节点本地名称
* @param[in]  const char *attribname: 属性名称
* @param[out] 无
* @return	 如果成功，返回属性值；如果失败，返回ERROR
*/
int xml_getfirsttagattribute_floatvalue_bylocalname(IN IXML_Node *parentnode,
							  IN const char *item, IN const char *attribname, IN float *float_value)
{
	char *ret = NULL;	
	IXML_Node *node = NULL;

	if(NULL == parentnode || NULL == item || NULL == attribname || NULL == float_value)
	{
		return ERROR;
	}

	/* 首先找到该节点 */
	node = xml_getfirstchildnode_bylocalname(parentnode, item);
	if(NULL == node)
	{
		return ERROR;
	}

	/* 然后获取属性 */
	ret = xml_getfirstattribval_byname(node, attribname);
	if(INVALID_NUM != check_num_valid(ret))
	{
		*float_value = atof(ret);
		return OK;
	}
	
	return ERROR;
}

/**@brief	 根据本地名称，获取标签值,并转换成整形
* @param[in]  IXML_Node * parentnode : 包含目的节点的父节点，或者是根节点，不必是直接的父节点
* @param[in]  const char *item : 节点本地名称
* @param[out] int *int_value: 转换后的整形值
* @return	 如果成功，返回OK；如果失败，返回ERROR
*/
int xml_getfirst_intvalue_bylocalname(IN IXML_Node *parentnode,
							  IN const char *item, OUT int *int_value)
{
	int ret = ERROR;	
	IXML_Node *node = NULL; //xml的body节点
	IXML_Node *textnode = NULL;

	if(NULL == parentnode || NULL == item)
	{
		return ERROR;
	}
	
	node = xml_getfirstchildnode_bylocalname(parentnode, item);
	if(NULL == node)
	{
		return ERROR;
	}
	
	textnode = ixmlNode_getFirstChild(node);
	if(NULL != textnode)
	{
		if(NULL != textnode->nodeValue)
		{
			if(INVALID_NUM != check_num_valid(textnode->nodeValue))
			{
				*int_value = atoi(textnode->nodeValue);
				ret = OK;
			}
			else
			{				
				ret = ERROR;
			}
		}
		else
		{			
			ret = ERROR;
		}
	}
	else
	{
		/*如果是空标签，会出现这种情况*/
		ret = ERROR;
	}
	
	return ret;
}


/**@brief	 根据本地名称，获取标签值,并转成浮点数
* @param[in]  IXML_Node * parentnode : 包含目的节点的父节点，或者是根节点，不必是直接的父节点
* @param[in]  const char *item : 节点本地名称
* @param[out] float *float_value : 转换后的浮点值
* @return	 如果成功，返回OK；如果失败，返回ERROR
*/
int xml_getfirst_floatvalue_bylocalname(IN IXML_Node *parentnode,IN const char *item, float *float_value)
{
	int ret = ERROR;	
	IXML_Node *node = NULL; //xml的body节点
	IXML_Node *textnode = NULL;

	if(NULL == parentnode || NULL == item)
	{
		return ERROR;
	}
	
	node = xml_getfirstchildnode_bylocalname(parentnode, item);
	if(NULL == node)
	{
		return ERROR;
	}
	
	textnode = ixmlNode_getFirstChild(node);
	if(NULL != textnode)
	{
		if(NULL != textnode->nodeValue)
		{
			if(INVALID_NUM != check_num_valid(textnode->nodeValue))
			{
				*float_value = atof(textnode->nodeValue);
				ret = OK;
			}
			else
			{				
				ret = ERROR;
			}
		}
		else
		{			
			ret = ERROR;
		}
	}
	else
	{
		/*如果是空标签，会出现这种情况*/
		ret = ERROR;
	}
	
	return ret;
}

/**@brief		判断字符串是否是有效的url
 * @1,URL长度不超过2048
 * @2,URL第一个字符必须为"/"
 * @3,URL非法字符需求暂不明确
 * @param[in]	char *p_url url字符串
 * @param[out]	
 * @return		有效:OK 无效: ERROR
 */
int is_url_valid(char *p_url)
{
	//int i = 0;
	int len = 0;

	if (NULL == p_url)
	{
		return ERROR;
	}

	//url最长长度暂时为2048
	len = strlen(p_url); 
	if (len > 2048 || len < 1)
	{
		return ERROR;
	}

	// URL第一个字符必须为"/"
	if ('/' != p_url[0])
	{
		return ERROR;
	}
     
	// 支持中文
	#if 0
	for (i = 0; i < len; i++)
	{
		// 暂时ASCII表中可见字符都作为有效的字符
		if (p_url[i] < '!' || p_url[i] > '~')
		{
			return ERROR;
		}
	}
	#endif
	

	return OK;
}

/**@brief		判断标签值是否是合法布尔值
 * @合法的布尔值是 true、false
 * @param[in]	char *p_tag 标签值
 * @param[out]	
 * @return		TRUE:标签值为布尔值FALSE : 非布尔值
 */
BOOL is_tagval_bool(char *p_tag)
{
	char tagval[8]; 

	if ((NULL == p_tag) || (strlen(p_tag) > 5))
	{
		XML_DBG( "p_tag is NULL!\n");
		return FALSE;
	}

	memset(tagval, 0, sizeof(tagval));
	strncpy(tagval, p_tag, MIN(sizeof(tagval),strlen(p_tag)));

	if((strcasecmp(tagval, "true") != 0) && (strcasecmp(tagval, "false") != 0))
	{
		XML_DBG( "p_tag illegal!\n");
		return FALSE;
	}

	return TRUE;
}

/**@brief		判断是否是合法的无符号的整型
* @param[in]		char *pValue : 字符串格式的数值表达式
* @param[out]	无
* @return		INVALD_NUM：返回输入的字符串为非法数字
				VALID_INT_NUM：返回输入的字符串为合法整数
				VALID_FLOAT：返回输入的字符串为合法浮点数
*/
BOOL check_unsigned_num_valid(char *str_num)
{
	size_t len = 0;
	char * str = NULL;
	if (NULL == str_num)
	{
		fprintf(stderr, "is_digit_num: str is null.\n");
		return FALSE;
	}

	str = str_num;
	len = strlen(str);

	while (len > 0)
	{
		if (*str < '0' || *str > '9')
		{
			return FALSE;
		}
		str++;
		len--;
	}

	return TRUE;
}


void safe_ixmlNodeList_free(IXML_NodeList **nList)
{
	IXML_NodeList *next;
	
	if ((NULL == nList) || (NULL == *nList))
	{
	    return;
	}

	while (*nList != NULL) 
	{
		next = (*nList)->next;
		free(*nList);
		*nList=NULL;
		*nList = next;
	}
}