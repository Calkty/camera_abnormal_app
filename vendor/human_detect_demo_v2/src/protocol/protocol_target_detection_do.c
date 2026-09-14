//#include <protocol_target_detection_do.h>
#include <protocol_status.h>
#include<config.h>
#include <syslog.h>


int isapi_get_target_detect_cfg(APP_CFG *target_detect_cfg)
{
    
    APP_CFG *event_cfg = NULL;
    int ret = 0;

    event_cfg = target_detect_cfg;


    
    ret = get_app_cfg(event_cfg);
    if (0 != ret)
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "get_app_cfg error\n");
        return PRO_DEV_ERR;
    }

    

    return PRO_OK;
    

}

int isapi_get_target_detect_cfg_V2(APP_CFG *target_detect_cfg, int chan)
{
    
    APP_CFG *event_cfg = NULL;
    int ret = 0;
    int chanInfo[MAXCHAN] = {0};
    int preChan = 0, valid_chan_num = 1;;

	ret = get_app_chan(HUMAN_APP_ID, chanInfo, &valid_chan_num, NULL);
	if(OK!=ret || valid_chan_num>MAXCHAN || valid_chan_num<1 || chanInfo[0]<1)
	{
	    syslog(LOG_ERR,"get_app_chan err ret %d, valid_chan_num %d, chanInfo[0] %d\n",ret, valid_chan_num, chanInfo[0]);
    	preChan = 1;
	}
	else
	{
		preChan = chanInfo[0];
	}

    if(preChan != chan)
    {
        syslog(LOG_ERR, "set_app_cfg_V2 chan[%d] not right with preChan[%d] filter\n", chan, preChan);
        return PRO_DEV_ERR;
    }

    event_cfg = target_detect_cfg; 
    ret = get_app_cfg_V2(event_cfg, chan);
    if (0 != ret)
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "get_app_cfg_V2 error\n");
        return PRO_DEV_ERR;
    }

    

    return PRO_OK;
    

}
int isapi_put_target_detect_cfg(APP_CFG *target_detect_cfg)
{
    
    int ret = 0;
    
    ret = set_app_cfg(target_detect_cfg);
    if (0 != ret)
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "set_app_cfg error\n");
        return PRO_DEV_ERR;
    }



    return PRO_OK;

}

int isapi_put_target_detect_cfg_V2(APP_CFG *target_detect_cfg, int chan)
{
    int chanInfo[MAXCHAN] = {0};
    int ret = 0;
    int preChan = 0, valid_chan_num = 1;;

	ret = get_app_chan(HUMAN_APP_ID, chanInfo, &valid_chan_num, NULL);
	if(OK!=ret || valid_chan_num>MAXCHAN || valid_chan_num<1 || chanInfo[0]<1)
	{
	    syslog(LOG_ERR,"get_app_chan err ret %d, valid_chan_num %d, chanInfo[0] %d\n",ret, valid_chan_num, chanInfo[0]);
    	preChan = 1;
	}
	else
	{
		preChan = chanInfo[0];
	}

    if(preChan != chan)
    {
        syslog(LOG_ERR, "set_app_cfg_V2 chan[%d] not right with preChan[%d] filter\n", chan, preChan);
        return PRO_DEV_ERR;
    }

    ret = set_app_cfg_V2(target_detect_cfg, chan);
    if (0 != ret)
    {
        opdevsdk_write_log(OPDEVSDK_LOG_ERROR, "set_app_cfg_V2 error\n");
        return PRO_DEV_ERR;
    }



    return PRO_OK;

}

