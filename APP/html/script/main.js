/*
this is a demo script for camera which allow watch the live, get data from camera and set data to it.
the detail of the data as xml format
<Config>
<normalizedScreenSize>  <!--req-->
<normalizedScreenWidth> <!-- req, xs:integer --></normalizedScreenWidth>
<normalizedScreenHeight> <!-- req, xs:integer --></normalizedScreenHeight>
</normalizedScreenSize>
<RegionCoordinatesList>
<RegionCoordinates>  <!-- req, -->
<positionX>      <!-- req, xs:integer;coordinate -->    </positionX>
<positionY>      <!-- req, xs:integer;coordinate  -->   </positionY>
</RegionCoordinates>
</RegionCoordinatesList>
<Test1>    <!-- opt, req, xs:string -->     </Test1>
<Test2>    <!-- opt, req, xs:string -->     </Test2>
<Test3>    <!-- opt, req, xs:string -->     </Test3>
<Test4>    <!-- opt, req, xs:string -->     </Test4>
</Config>
*/
$(function () {
  //custom protocol
  var HTTP = location.protocol + '//';
  //Host Address
  var HOST = location.hostname;
  //custom protocol port
  var PORT = '80';
  //video Protocol
  var RTSP = 'rtsp://';
  //video path
  var LIVE = '/ISAPI/streaming/channels/102';
  //Video Plugin
  var Plugin = null;
  //login session info
  // var SESSION = window.parent.getUserSession();
  //user info, security upgrade. it's no safe in stream URL, replace by session type;
  // var USERINFO = window.parent.getUserAuth(true);
  //stream auth
  $.support.cors = true;
  var iHttpPort = 80;
  if (location.port != "") {
    iHttpPort = location.port;
  } else if (location.protocol == "https:") {
    iHttpPort = 443;
  }
  var szSessionTag = localStorage.getItem("jumpTagInfo/" + HOST + ":" + iHttpPort);
  if (szSessionTag) {
    $.ajaxSetup({
      beforeSend: function (xhr, questParam) {
        xhr.setRequestHeader("SessionTag", szSessionTag);
      }
    });
  }
  var AUTH = '';

  function getToken() {
    $.ajax({
      url: HTTP + HOST + ":" + PORT + "/ISAPI/Security/token?format=json",
      type: "GET",
      async: false,
      success: function (oData) {
        //text param
        if (oData && oData.Token) {
          AUTH = oData.Token.value;
        }
      }
    });
  }
  var szSession = $.cookie("WebSession");
  if (szSession) { //不存在说明是token认证
    AUTH = szSession;
  } else {
    getToken();
  }
  //screen normalized max Length
  var screenMaxWidth = 1000;
  var screenMaxHeight = 1000;

  //allow IE cross site
  $.support.cors = true;

  //init video, data...
  setTimeout(function () {
    initLiveView();
  }, 500);
  var rectCanvas = RectCanvas("liveviewCanvas");
  rectCanvas.initRectCanvas(600, 450);

  // io data: {id: '1', alarmName: 'A->1'}
  const aIOOutList = [];
  getIoOutputs().then((res) => aIOOutList.push(...res));

  // 获取通道
  getDeviceChannelList().then((deviceChannels) => {
    if (deviceChannels.length === 0) {
      console.error('There is no channel to meet the requirements');
      return;
    }
    // 挂载通道, 默认选中第一个通道
    appendChannel(deviceChannels);

    const firstButton = document.getElementById('drawBtn' + deviceChannels[0].id);
    if (firstButton) {
      firstButton.classList.add('selected');
      getCurChannelInfo(deviceChannels[0].id); // 获取第一个通道的数据
    }
  });
  
  // 获取io端口
  function getIoOutputs() {
    const getIOReq = () => {
      return $.ajax({
        url: HTTP + HOST + ":" + PORT + '/ISAPI/System/IO/outputs',
        type: "GET",
        dataType: "xml",
        async: false
      });
    };
    return getIOReq().then((xmlData) => {
      const aIOList = [];
      $(xmlData).find("IOOutputPort").each(function () {
        const $this = $(this);
        const id = $this.find('id').eq(0).text();
        aIOList.push({
          id: id,
          alarmName: "A->" + id
        });
      });
      return aIOList;
    });
  }

  /** get device channel  */
  async function getDeviceChannelList() {
    // 通道预处理
    const dealChannelData = (dataXML) => {
      const channelArray = [];
      $(dataXML).find('VideoInputChannel').each(function() {
        const id = $(this).find('id').text();
        const name = $(this).find('channelDescription').text();
        channelArray.push({ id: id, name: name });
      });
      return channelArray;
    };

    // 通道能力检测
    const testChannel = (channleId) => {
      const curURL = `/ISAPI/Custom/OpenPlatform/extern/cameraAbnormal/capabilities?format=json&chanID=${channleId}`;
      return $.ajax({
        url: HTTP + HOST + ":" + PORT + curURL,
        type: "GET",
        dataType: "json",
        async: false
      });
    };

    const getChannel = () => {
      return new Promise(resolve => {
        $.ajax({
          url: HTTP + HOST + ":" + PORT + "/ISAPI/System/Video/inputs/channels",
          type: "GET",
          async: true,
          success: (dataXML) => {
            console.log("🚀 ~ getChannel ~ oData:", dataXML);
            const iChannel = dealChannelData(dataXML); // 预处理当前通道数据
            // 获取通道检测请求
            const reqList = iChannel.map(({id}) => {
              return testChannel(id);
            });
            // 测试通道是否可请求通
            Promise.allSettled(reqList).then((channelList) => {
              const curChannelList = []; // 最终合格通道
              // 过滤不符合要求的通道
              channelList.forEach((item, index) => {
                const { status } = item;
                if (status === 'fulfilled') {
                  curChannelList.push(iChannel[index]);
                }
              });
              console.log("🚀 ~ channelList.forEach ~ curChannelList:", curChannelList);
              resolve(curChannelList);
            });
          }
        });
      });
    };
    return await getChannel();
  }

  let deviceCurChannels = [];

  /** append channel  */
  function appendChannel(deviceChannels) {
    deviceCurChannels = deviceChannels;
    const channelsDiv = document.getElementById('channels');

    for (const channel of deviceChannels) {
      const span = document.createElement('span');
      span.className = 'drawButtons';

      const inputButton = document.createElement('input');
      inputButton.type = 'button';
      inputButton.value = channel.name + channel.id;
      inputButton.id = 'drawBtn' + channel.id;

      // 通道绑定事件
      inputButton.onclick = function() {
        // 重置颜色
        const buttons = document.querySelectorAll('#channels .selected');
        buttons.forEach(btn => btn.classList.remove('selected'));
        this.classList.add('selected');

        // 获取当前通道下数据
        getCurChannelInfo(channel.id);
      };

      span.appendChild(inputButton);
      span.style.marginTop = '5px';
      channelsDiv.appendChild(span);
    }
  }
  
  /** get current channel info */
  function getCurChannelInfo(channel) {
    console.log("🚀 ~ getCurChannel ~ channel:", channel);
    getParamCap(channel);
  }

  let currentChannel = -1;
  /** get capabilities from device */
  function getParamCap(channel) {
    let curURL = '/ISAPI/Custom/OpenPlatform/extern/cameraAbnormal/capabilities?format=json';
    if (channel > 0) {
      currentChannel = channel;
      curURL = `/ISAPI/Custom/OpenPlatform/extern/cameraAbnormal/capabilities?format=json&chanID=${channel}`;
    }
    $("#params").html("");
    $.ajax({
      url: HTTP + HOST + ":" + PORT + curURL,
      type: "GET",
      dataType: "json",
      async: false,
      success: (oJson) => {
        var data = oJson.humanDetectDemoCap;
        if (data.RegionCap) {
          data.RegionCap.maxSize && (rectCanvas.iMaxPointNum = parseInt(data.RegionCap.maxSize, 10) || 10);
          data.RegionCap.minSize && (rectCanvas.iMinPointNum = (parseInt(data.RegionCap.minSize, 10) - 1) || 2);
        }
        if (data.RegionCap) {
          data.RegionCap.maxSize && (rectCanvas.iMaxPointNum = parseInt(data.RegionCap.maxSize, 10) || 10);
          data.RegionCap.minSize && (rectCanvas.iMinPointNum = (parseInt(data.RegionCap.minSize, 10) - 1) || 2);
        }
        if (data.LinkageCap) {
          $("#params").append("<div><span class='paramsTitle'>联动配置</span><div id='linkContent'></div></div>");
          if (data.LinkageCap.isSupportAudioOut) {
            $("#linkContent").append("<div><span><input id='audioOutlink' type='checkbox' class='checkbox'/><label class='check-label margin-left5'>语言报警</label></span></div>");
          }
          if (data.LinkageCap.isSupportAlarmOut) {
            $("#linkContent").append("<div id='alarmOutlink' style='display: flex;'><p id='alarmOutlinkTitle' style='font-size: 14px;'>联动报警输出</p></div>");
            aIOOutList.forEach(({id}) => {
              $("#alarmOutlinkTitle").append(`<span style='margin-left: 10px;font-size: 14px;'><input id='alarmOutlink${id}' type='checkbox' class='checkbox'/><label class='check-label margin-left5'>A-${id}</label></span>`);
            });
          }
          if (data.LinkageCap.isSupportCenter) {
            $("#linkContent").append("<div><span><input id='centerOutlink' type='checkbox' class='checkbox'/><label class='check-label margin-left5'>上传中心</label></span></div>");
          }
          if (data.LinkageCap.isSupportStorage) {
            $("#linkContent").append("<div><span><input id='storeOutlink' type='checkbox' class='checkbox'/><label class='check-label margin-left5'>联动存储</label></span></div>");
          }
          if (data.LinkageCap.isSupportRecord) {
            $("#linkContent").append("<div id='recordOutlink' style='display: flex;'><p id='recordOutlinkTitle' style='font-size: 14px;'>联动录像</p></div>");
            aIOOutList.forEach(({id}) => {
              $("#recordOutlinkTitle").append(`<span style='margin-left: 10px;font-size: 14px;'><input id='recordOutlink${id}' type='checkbox' class='checkbox'/><label class='check-label margin-left5'>A-${id}</label></span>`);
            });
          }
          if (data.LinkageCap.isSupportCapture) {
            $("#linkContent").append("<div id='captureOutlink' style='display: flex;'><p id='captureOutlinkTitle' style='font-size: 14px;'>联动抓拍</p></div>");
            aIOOutList.forEach(({id})=> {
              $("#captureOutlinkTitle").append(`<span style='margin-left: 10px;font-size: 14px;'><input id='captureOutlink${id}' type='checkbox' class='checkbox'/><label class='check-label margin-left5'>A-${id}</label></span>`);
            });
          }
        }
        getParam(channel);
      },
      error: function (xhr, text) {
        tip(text);
      }
    });
  }
  /** get param from device */
  async function getParam(channel) {
    clearRect();
    if (Plugin) {
      getToken();
      await Plugin.JS_Stop(0);
      initLiveView();
    }
    $.ajax({
      url: HTTP + HOST + ":" + PORT + `/ISAPI/Custom/OpenPlatform/extern/cameraAbnormal/config?format=json&chanID=${channel}`,
      type: "GET",
      dataType: "json",
      async: false,
      success: function (oJson) {
        var data = oJson.humanDetectDemo;
        $("#enable").prop("checked", data.enabled);
        //draw the Polygon
        if (data.humanDetectDemoRegion) {
          var Points = data.humanDetectDemoRegion.RegionCoordinates || [];
          rectCanvas.setPolygonPoints(Points, screenMaxWidth, screenMaxHeight);
        }
        if (oJson.EventTrigger) {
          $.each(oJson.EventTrigger.EventTriggerNotification, function (i, o) {
            const aOutputIOPortID = o.outputIOPortID;

            if ('audioOut' === o.id) {
              $("#audioOutlink").prop("checked", true);
            }
            if ('alarmOut' === o.id) {
              aOutputIOPortID.forEach(item => {
                const curId = item.id;
                $("#alarmOutlink" + curId).prop("checked", true);
              });
            }
            if ('uploadCenter' === o.id) {
              $("#centerOutlink").prop("checked", o.enable);
            }
            if ('Storage' === o.id) {
              $("#storeOutlink").prop("checked", o.enable);
            }
            if ('Record' === o.id) {
              const aRecordChan = o.recordChan;
              aRecordChan.forEach(item => {
                const curId = item.chan;
                $("#recordOutlink" + curId).prop("checked", true);
              });
            }
            if ('Capture' === o.id) {
              const aCaptureChan = o.captureChan;
              aCaptureChan.forEach(item => {
                const curId = item.chan;
                $("#captureOutlink" + curId).prop("checked", true);
              });
            }
          });
        }
      },
      error: function (xhr, text) {
        tip(text);
      }
    });
  }

  /** use quicktime plugin */
  function initLiveView() {
    $("#liveview").html("");
    Plugin = new JSPlugin({
      szId: "liveview",
      iType: "1",
      iWidth: $("#liveview").width(),
      iHeight: $("#liveview").height(),
      iMaxSplit: 8,
      iCurrentSplit: 1,
      szBasePath: "./script/"
    });

    //Please fill in the port number based on the actual parameters set by the device
    let curPost = '102';
    // Take the aisle or not
    if (currentChannel > 0) {
      curPost = `${currentChannel}02`;
    }
    var szUrl = "ws://" + HOST + ":7681/" + curPost;
    console.log("🚀 ~ initLiveView ~ szUrl:", szUrl);
    Plugin.JS_Play(szUrl, {
      sessionID: AUTH,
      token: AUTH
    }, 0);
  }
  
  /** draw rect on canvas */
  function RectCanvas(id) {
    //canvas element
    this.canvasElement = $("#" + id)[0];
    try {
      //browser which support canvas
      this.context2D = this.canvasElement.getContext("2d");
    } catch (e) {
      //for ie8
      G_vmlCanvasManager.init($("#" + id).parent()[0]);
      this.canvasElement = $("#" + id)[0];
      this.context2D = this.canvasElement.getContext("2d");
    }
    //canvas width and height
    this.width = 0;
    this.height = 0;

    //line color
    this.lineColor = '#FF0000';

    //allow draw
    this.allowDraw = false;
    var that = this;

    //points in rect
    var rectPoints = [];
    //points in Polygon
    this.iMaxPointNum = 10;
    this.iMinPointNum = 2;
    var aPoint = [];
    var szStatus = "default";
    var bChoosed = false;
    var m_iIndexChoosePoint = -1; //判断鼠标点击选中图形上点的索引
    var m_iDriftStartX = 0; //drag起始X坐标
    var m_iDriftStartY = 0; //drag起始Y坐标
    var m_oEdgePoints = {
      top: {
        x: 0,
        y: 0
      },
      left: {
        x: 0,
        y: 0
      },
      right: {
        x: 0,
        y: 0
      },
      bottom: {
        x: 0,
        y: 0
      }
    }; //多边形中用于判断边界

    //init canvas width, height and event
    this.initRectCanvas = function (width, height) {
      this.setPanelSize(width, height);
      _clear();
      _bindEvent();
    };

    //draw lines, will clear pre lines first
    this.drawLines = function (aMaps) {
      _clear();
      var iLength = aMaps.length;
      for (var i = 0; i < iLength; i++) {
        _drawLine({
          x: aMaps[i].start.x,
          y: aMaps[i].start.y
        }, {
          x: aMaps[i].end.x,
          y: aMaps[i].end.y
        });
      }
    };

    //set canvas width and height
    this.setPanelSize = function (width, height) {
      that.width = width;
      that.height = height;
    };

    //get points in rect, iNormalized is max size of point
    this.getRectPoints = function (iNormalizedWidth, iNormalizedHeight) {
      var len = rectPoints.length;
      var normalizedPoints = [];
      for (var i = 0; i < len; i++) {
        normalizedPoints.push({
          x: Math.ceil(rectPoints[i].x * iNormalizedWidth / that.width),
          y: Math.ceil(rectPoints[i].y * iNormalizedHeight / that.height)
        });
      }
      return normalizedPoints;
    };
    this.getPolygonPoints = function (iNormalizedWidth, iNormalizedHeight) {
      var len = aPoint.length;
      var normalizedPoints = [];
      for (var i = 0; i < len; i++) {
        normalizedPoints.push({
          x: Math.ceil(aPoint[i][0] * iNormalizedWidth / that.width),
          y: Math.ceil(aPoint[i][1] * iNormalizedHeight / that.height)
        });
      }
      return normalizedPoints;
    };

    //set points to canvas
    this.setRectPoints = function (rect, iNormalizedWidth, iNormalizedHeight) {
      var lines = [];
      var len = rect.length;
      for (var i = 0; i < len; i++) {
        lines.push({
          start: {
            x: rect[i].x * that.width / iNormalizedWidth,
            y: rect[i].y * that.height / iNormalizedHeight
          },
          end: {
            x: rect[(i + 1) == len ? 0 : (i + 1)].x * that.width / iNormalizedWidth,
            y: rect[(i + 1) == len ? 0 : (i + 1)].y * that.height / iNormalizedHeight
          }
        });
      }
      return lines;
    };
    this.setPolygonPoints = function (points, iNormalizedWidth, iNormalizedHeight) {
      if (points && points.length) {
        aPoint.length = 0;
        var len = points.length;
        for (var i = 0; i < len; i++) {
          aPoint.push([parseInt(points[i].x, 10) / iNormalizedWidth * that.width, parseInt(points[i].y, 10) / iNormalizedHeight * that.height]);
        }
        reDraw();
      }
    };

    //clear the rect
    this.clearRect = function () {
      _clear();
      aPoint.length = 0;
    };

    //draw line
    function _drawLine(start, end) {
      if (start.x == end.x && start.y == end.y) {
        return;
      }

      //line property
      that.context2D.strokeStyle = that.lineColor;
      that.context2D.lineJoin = "round";
      that.context2D.lineWidth = 1;

      //draw line
      that.context2D.beginPath();
      that.context2D.moveTo(start.x, start.y);
      that.context2D.lineTo(end.x, end.y);
      that.context2D.stroke();
    }

    //clear lines
    function _clear() {
      that.context2D.clearRect(0, 0, that.canvasElement.width, that.canvasElement.height);
    }

    //bind canvas mouseup and mousedown event
    //添加坐标
    function addPoint(iMouseDownX, iMouseDownY) {
      if (aPoint.length < that.iMaxPointNum) {
        aPoint.push([iMouseDownX, iMouseDownY]);
      }
      if (aPoint.length === that.iMaxPointNum) {
        setPointInfo(aPoint);
      }
    }
    //计算边缘坐标
    function setPointInfo(aPoint) {
      for (let i = 0, iLen = aPoint.length; i < iLen; i++) {
        if (i === 0) {
          m_oEdgePoints.top.x = aPoint[i][0];
          m_oEdgePoints.top.y = aPoint[i][1];
          m_oEdgePoints.left.x = aPoint[i][0];
          m_oEdgePoints.left.y = aPoint[i][1];
          m_oEdgePoints.right.x = aPoint[i][0];
          m_oEdgePoints.right.y = aPoint[i][1];
          m_oEdgePoints.bottom.x = aPoint[i][0];
          m_oEdgePoints.bottom.y = aPoint[i][1];
        } else {
          if (aPoint[i][1] < m_oEdgePoints.top.y) {
            m_oEdgePoints.top.x = aPoint[i][0];
            m_oEdgePoints.top.y = aPoint[i][1];
          }
          if (aPoint[i][0] > m_oEdgePoints.right.x) {
            m_oEdgePoints.right.x = aPoint[i][0];
            m_oEdgePoints.right.y = aPoint[i][1];
          }
          if (aPoint[i][1] > m_oEdgePoints.bottom.y) {
            m_oEdgePoints.bottom.x = aPoint[i][0];
            m_oEdgePoints.bottom.y = aPoint[i][1];
          }
          if (aPoint[i][0] < m_oEdgePoints.left.x) {
            m_oEdgePoints.left.x = aPoint[i][0];
            m_oEdgePoints.left.y = aPoint[i][1];
          }
        }
      }
    }

    //画点移动
    function move(iMouseMoveX, iMouseMoveY) {
      if (aPoint.length < that.iMaxPointNum && aPoint.length > 0) {
        _clear();
        that.context2D.strokeStyle = that.lineColor;
        that.context2D.globalAlpha = 1;
        var i = 0;
        var iLen = 0;
        for (i = 0, iLen = aPoint.length; i < iLen; i++) {
          that.context2D.beginPath();
          that.context2D.arc(aPoint[i][0], aPoint[i][1], 3, 0, 360, false);
          that.context2D.fillStyle = that.lineColor;
          that.context2D.closePath();
          that.context2D.fill();
        }
        that.context2D.beginPath();
        that.context2D.moveTo(aPoint[0][0], aPoint[0][1]);
        for (i = 0, iLen = aPoint.length; i < iLen; i++) {
          if (i !== 0) {
            that.context2D.lineTo(aPoint[i][0], aPoint[i][1]);
          }
        }
        that.context2D.lineTo(iMouseMoveX, iMouseMoveY);
        that.context2D.closePath();
        that.context2D.stroke();
      }
    }
    //绘制已有的图形
    function reDraw() {
      if (aPoint.length > 0) {
        _clear();
        that.context2D.fillStyle = that.lineColor;
        that.context2D.strokeStyle = that.lineColor;
        that.context2D.globalAlpha = 1;
        let i = 0;
        let iLen = 0;
        if (bChoosed) {
          for (i = 0, iLen = aPoint.length; i < iLen; i++) {
            that.context2D.beginPath();
            that.context2D.arc(aPoint[i][0], aPoint[i][1], 3, 0, 360, false);
            that.context2D.fillStyle = that.lineColor;
            that.context2D.closePath();
            that.context2D.fill();
          }
        }
        that.context2D.beginPath();
        that.context2D.moveTo(aPoint[0][0], aPoint[0][1]);
        for (i = 0, iLen = aPoint.length; i < iLen; i++) {
          if (i !== 0) {
            that.context2D.lineTo(aPoint[i][0], aPoint[i][1]);
          }
        }
        that.context2D.lineTo(aPoint[0][0], aPoint[0][1]);
        that.context2D.stroke();
        that.context2D.closePath();
      }
    }
    //判断是否在图形边界圆点内
    function inArc(iPointX, iPointY, iRadius) {
      var bRet = false;
      for (var i = 0, iLen = aPoint.length; i < iLen; i++) {
        var iDistance = Math.sqrt((iPointX - aPoint[i][0]) * (iPointX - aPoint[i][0]) + (iPointY - aPoint[i][1]) * (iPointY - aPoint[i][1]));
        if (iDistance < iRadius) {
          bRet = true;
          m_iIndexChoosePoint = i;
          break;
        }
      }
      return bRet;
    }
    //判断是否在图形内
    function inShape(iPointX, iPointY) {
      var bRet = false;
      var iLen = aPoint.length;
      for (var i = 0, j = iLen - 1; i < iLen; j = i++) {
        if (((aPoint[i][1] > iPointY) !== (aPoint[j][1] > iPointY))
					&& (iPointX < (aPoint[j][0] - aPoint[i][0]) * (iPointY - aPoint[i][1]) / (aPoint[j][1] - aPoint[i][1]) + aPoint[i][0])) {
          bRet = !bRet;
        }
      }
      return bRet;
    }
    //鼠标点击选择图形时调用获取坐标偏差
    function getMouseDownPoints(iMouseDownX, iMouseDownY) {
      m_iDriftStartX = iMouseDownX;
      m_iDriftStartY = iMouseDownY;
    }
    //平移图形
    function drag(iPointX, iPointY) {
      var iLength = aPoint.length;
      var i = 0;
      for (i = 0; i < iLength; i++) {
        if (aPoint[i][0] + iPointX - m_iDriftStartX > 600 || aPoint[i][1] + iPointY - m_iDriftStartY > 450
					|| aPoint[i][0] + iPointX - m_iDriftStartX < 0 || aPoint[i][1] + iPointY - m_iDriftStartY < 0) {
          m_iDriftStartX = iPointX;
          m_iDriftStartY = iPointY;
          return;
        }
      }
      for (i = 0; i < iLength; i++) {
        aPoint[i][0] = aPoint[i][0] + iPointX - m_iDriftStartX;
        aPoint[i][1] = aPoint[i][1] + iPointY - m_iDriftStartY;
      }
      m_iDriftStartX = iPointX;
      m_iDriftStartY = iPointY;
      setPointInfo(aPoint);
      reDraw();
    }
    //拉伸图形
    function stretch(iPointX, iPointY) {
      if (m_iIndexChoosePoint !== -1) {
        aPoint[m_iIndexChoosePoint][0] = iPointX;
        aPoint[m_iIndexChoosePoint][1] = iPointY;
      }
      setPointInfo(aPoint);
      reDraw();
    }

    function arcClosePoint() {
      if (aPoint.length) {
        that.context2D.beginPath();
        that.context2D.arc(aPoint[aPoint.length - 1][0], aPoint[aPoint.length - 1][1], 3, 0, 360, false);
        that.context2D.fillStyle = that.lineColor;
        that.context2D.closePath();
        that.context2D.fill();
      }
    }

    function _bindEvent() {
      var iMouseDownX = 0;
      var iMouseDownY = 0;
      //mouse start draw
      $(that.canvasElement).on("mousedown", function (e) {
        // if (that.allowDraw) {
        // 	var tmpX = event.pageX - ($(that.canvasElement).offset()).left;
        // 	var tmpY = event.pageY - ($(that.canvasElement).offset()).top;
        // 	start = {
        // 		x: tmpX,
        // 		y: tmpY
        // 	};
        // 	isStart = true;
        // }
        iMouseDownX = e.offsetX;
        iMouseDownY = e.offsetY;
        if (e.button === 0) { //鼠标左键
          szStatus = "draw";
          if (that.allowDraw) {
            addPoint(iMouseDownX, iMouseDownY);
            if (aPoint.length === that.iMaxPointNum) {
              that.allowDraw = false; //完成绘制
              arcClosePoint();
            }
          } else {
            //判断是否在圆点内
            if (inArc(e.offsetX, e.offsetY, 5)) {
              szStatus = "stretch";
              bChoosed = true;
            }
            //未选中圆点，则判断是否选中某个图形
            if (szStatus !== "stretch") {
              if (inShape(e.offsetX, e.offsetY)) { //如果图像属性为不可编辑，则不算选中移动
                bChoosed = true;
                getMouseDownPoints(e.offsetX, e.offsetY);
                szStatus = "drag";
              } else {
                bChoosed = false;
              }
            }
          }
        } else if (e.button === 2) { //鼠标右键闭合图形
          if (that.allowDraw && aPoint.length >= that.iMinPointNum) {
            addPoint(iMouseDownX, iMouseDownY);
            that.allowDraw = false; //完成绘制
            arcClosePoint();
          }
        }
      });

      $(that.canvasElement).on("mousemove", function (e) {
        // if (isStart) {
        // 	var tmpX = event.pageX - ($(that.canvasElement).offset()).left;
        // 	var tmpY = event.pageY - ($(that.canvasElement).offset()).top;
        // 	//non ie8 act fast, show the rect immediately
        // 	//if(!G_vmlCanvasManager) {
        // 	that.drawLines(_getRect(tmpX, tmpY));
        // 	//}
        // }
        if (aPoint.length) {
          // _drawLine({
          // 	x: iMouseDownX,
          // 	y: iMouseDownY
          // }, {
          // 	x: e.offsetX,
          // 	y: e.offsetY

          // });
          if (that.allowDraw) {
            move(e.offsetX, e.offsetY);
          } else {
            if (szStatus === "drag") {
              drag(e.offsetX, e.offsetY);
            } else if (szStatus === "stretch") {
              stretch(e.offsetX, e.offsetY);
            }
          }
        }
      });

      //mouse up end draw
      $(that.canvasElement).on("mouseup", function (event) {
        szStatus = "default";
      });
      $(that.canvasElement).on("mouseout", function (event) {
        szStatus = "default";
      });
      $(that.canvasElement)[0].oncontextmenu = function () {
        return false;
      };
      //translate draw rect points to line
      // function _getRect(x, y) {
      // 	rectPoints.length;
      // 	var rect = [{
      // 			x: start.x,
      // 			y: start.y
      // 		},
      // 		{
      // 			x: x,
      // 			y: start.y
      // 		},
      // 		{
      // 			x: x,
      // 			y: y
      // 		},
      // 		{
      // 			x: start.x,
      // 			y: y
      // 		}
      // 	];
      // 	rectPoints = rect;
      // 	var lines = [];
      // 	var len = rect.length;
      // 	for (var i = 0; i < len; i++) {
      // 		lines.push({
      // 			start: rect[i],
      // 			end: rect[(i + 1) == len ? 0 : (i + 1)]
      // 		});
      // 	}
      // 	return lines;
      // }
    }

    return that;
  }

  //demo interface for browser window environment
  var demoInterface = {};

  //draw button status
  var drawBtnStart = false;
	
  /** for draw button */
  function draw() {
    // $("#drawBtn").val(drawBtnStart ? "Start Draw" : "End Draw");
    clearRect();
    rectCanvas.allowDraw = true;
  }

  /** for clear button */
  function clearRect() {
    rectCanvas.clearRect();
  }

  /** save param to device */
  function setParam() {
    //data as xml format
    var oData = {
      "humanDetectDemo": {
        "enabled": $("#enable").is(":checked"),
        "humanDetectDemoRegion": {
          "RegionCoordinates": rectCanvas.getPolygonPoints(screenMaxWidth, screenMaxHeight)
        }
      },
      "EventTrigger": {
        "EventTriggerNotification": []
      }
    };
    if ($("#audioOutlink").length && $("#audioOutlink").is(":checked")) {
      oData.EventTrigger.EventTriggerNotification.push({
        "id": "audioOut",
        "notificationMethod": "audioOut"
      });
    }
    if ($("#alarmOutlinkTitle").length) {
      const aChecked = [];
      aIOOutList.forEach(({id}) => {
        const szId = '#alarmOutlink' + id;
        if ($(szId).is(":checked")) {
          aChecked.push({id: Number(id)});
        }
      });

      oData.EventTrigger.EventTriggerNotification.push({
        "id": "alarmOut",
        "notificationMethod": "IO",
        "outputIOPortID": aChecked,
        "enable": "true"
      });
    }
    if ($("#centerOutlink").length && $("#centerOutlink").is(":checked")) {
      oData.EventTrigger.EventTriggerNotification.push({
        "id": "uploadCenter",
        "notificationMethod": "uploadCenter",
        "notificationRecurrence": "beginning",
        "enable": "true"
      });
    }
    if ($("#storeOutlink").length && $("#storeOutlink").is(":checked")) {
      oData.EventTrigger.EventTriggerNotification.push({
        "id": "Storage",
        "notificationMethod": "Storage",
        "notificationRecurrence": "beginning",
        "enable": "true"
      });
    }
    if ($("#recordOutlinkTitle").length) {
      const aChecked = [];
      aIOOutList.forEach(({id}) => {
        const szId = '#recordOutlink' + id;
        if ($(szId).is(":checked")) {
          aChecked.push({chan: Number(id)});
        }
      });

      oData.EventTrigger.EventTriggerNotification.push({
        "id": "Record",
        "notificationMethod": "Record",
        "notificationRecurrence": "beginning",
        "recordChan": aChecked,
        "enable": "true"
      });
    }
    if ($("#captureOutlinkTitle").length) {
      const aChecked = [];
      aIOOutList.forEach(({id}) => {
        const szId = '#captureOutlink' + id;
        if ($(szId).is(":checked")) {
          aChecked.push({chan: Number(id)});
        }
      });

      oData.EventTrigger.EventTriggerNotification.push({
        "id": "Capture",
        "notificationMethod": "Capture",
        "notificationRecurrence": "beginning",
        "captureChan": aChecked,
        "enable": "true"
      });
    }
    oData = JSON.stringify(oData);
    let curURL = "/ISAPI/Custom/OpenPlatform/extern/cameraAbnormal/config?format=json";
    if (currentChannel > 0) {
      curURL = `/ISAPI/Custom/OpenPlatform/extern/cameraAbnormal/config?format=json&chanID=${currentChannel}`;
    }
    $.ajax({
      url: HTTP + HOST + ":" + PORT + curURL,
      type: "PUT",
      async: false,
      dataType: "json",
      data: oData,
      success: function () {
        tip("保存成功！");
      },
      error: function () {
        tip("保存失败！");
      }
    });
  }

  /** show the prompt box */
  function tip(szContent) {
    if ("undefined" === typeof szContent) {
      szContent = "";
    }
    var dt = artDialog.get("dialogtip");
    if (typeof dt !== "undefined") {
      dt.close();
    }
    $.dialog({
      id: "dialogtip",
      title: "提示",
      content: szContent,
      zIndex: 20000,	// 提示框默认的index应该比弹出框的10000高
      width: 200,
      height: 100,
      left: "100%",
      top: "100%",
      fixed: true,
      drag: false,
      resize: false,
      lock: false,
      duration: 0,
      time: 3,
      close: function () {
        if (this.mouseover) {
          return false;
        }
        var d = $.dialog.list["dialogtip"];
        d.DOM.se[0].style.height = "";
        d.DOM.sw[0].style.height = "";
      },
      mouseout: function () {
        this.time(3);
      }
    });
    var d = artDialog.get("dialogtip");
    d.DOM.se[0].style.height = "1px";
    d.DOM.sw[0].style.height = "1px";
    d.position("100%", "100%");
  }
  demoInterface.draw = draw;
  demoInterface.clearRect = clearRect;
  demoInterface.setParam = setParam;
  window.demoInterface = demoInterface;
});
