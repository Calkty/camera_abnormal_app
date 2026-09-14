

#include "opdevsdk_common_basic.h"
#include "opdevsdk_common_http.h"
#include <cgi_page.h>



#define GET          0x2             /**< GET method  */
#define PUT          0x20            /**< PUT method  */


#define ID_NAME 64


typedef struct
{
	OP_DEVSDK_REQ_DES *req;         /*请求信息*/
	OP_DEVSDK_RESP_DES *resp;       /*应答信息*/
	int sockfd;              /*透传isapi专用*/
}WEB_DES;

typedef struct tag_node_t
{
    const char         *service_name;
    int (*proecess)(struct tag_node_t *dstnode, WEB_DES *web, char (*id)[ID_NAME], char *remain_path,  CGI_PAGE *page );   
    struct tag_node_t  *prenode;
    struct tag_node_t   *nextnode;
    int           bservice;   /*service -1; source - 0*/
    int           method;     /*此节点支持的http 方法get put....*/
	int			  format;	  /* FORMAT_XML | FORMAT_JSON,必填 */
}node_t;

int isapi_target_detect_ext(node_t *dstnode, WEB_DES *webinfo, char *remain_path, CGI_PAGE *page);
int isapi_target_detect_ext_V2(node_t *dstnode, WEB_DES *webinfo, char *remain_path, CGI_PAGE *page, int chan);
int isapi_target_detect_capabilities(node_t *dstnode, WEB_DES *webinfo, char *remain_path, CGI_PAGE *page);


