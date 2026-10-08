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
  //Currently selected channel. Declared here because the detection overlay
  //starts polling before the channel list has been fetched.
  let currentChannel = -1;
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
  rectCanvas.initRectCanvas(1920, 1080);

  // detection boxes: separate layer, polled independently of the video player
  var detectOverlay = DetectOverlay("detectCanvas");
  detectOverlay.setEnabled($("#showDetectBox").is(":checked"));
  $("#showDetectBox").on("change", function () {
    detectOverlay.setEnabled(this.checked);
  });

  // 视频区按窗口大小等比缩放；resize 做 120ms 防抖
  fitLiveview();
  $("#fitWindow").on("change", function () {
    fitLiveview();
  });
  var fitTimer = null;
  $(window).on("resize", function () {
    if (fitTimer) {
      clearTimeout(fitTimer);
    }
    fitTimer = setTimeout(fitLiveview, 120);
  });
  $(window).on("beforeunload", function () {
    detectOverlay.stop();
  });
  detectOverlay.start();

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
    // drop the previous channel's boxes, polling continues
    detectOverlay.reset();
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

  /**
   * 让视频区自适应窗口，解决「必须用浏览器缩放才能看全」的问题。
   *
   * 只加 CSS transform 缩放，**不改布局尺寸**：播放插件、规则画布、检测框图层
   * 依旧工作在 1920x1080 坐标系里，canvas 的鼠标 offsetX 也不受 transform 影响，
   * 所以规则绘制/拖动与检测框位置都不会错位。
   */
  function fitLiveview() {
    var box = document.getElementById("liveviewScale");
    var block = $(".liveviewBlock")[0];
    if (!box || !block) {
      return;
    }
    /* 取消勾选「适应窗口」= 回到原来的 1:1（放不下就靠页面滚动，不能裁掉画面） */
    if (!$("#fitWindow").is(":checked")) {
      block.style.transform = "";
      box.style.width = "";
      box.style.height = "";
      box.style.overflow = "";
      return;
    }

    var BASE_W = 1920;
    var BASE_H = 1080;
    /* 文档坐标系里的顶部偏移，与页面滚动位置无关 */
    var top = box.getBoundingClientRect().top + (window.pageYOffset || 0);
    var availW = document.documentElement.clientWidth - 40 - 16; /* .content 的 padding-left */
    var availH = window.innerHeight - top - 16;
    var scale = Math.min(availW / BASE_W, availH / BASE_H);

    if (!isFinite(scale) || scale <= 0) {
      return;
    }
    /* 不放大；最小缩到 25%，再小就没法点规则点了 */
    scale = Math.min(1, Math.max(0.25, scale));
    block.style.transform = "scale(" + scale + ")";
    box.style.width = Math.round(BASE_W * scale) + "px";
    box.style.height = Math.round(BASE_H * scale) + "px";
    /* 缩放后画面才能刚好放进 wrapper，多出来的 1920 布局宽度必须裁掉 */
    box.style.overflow = "hidden";
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
    let curPost = '103';
    // Take the aisle or not
    if (currentChannel > 0) {
      curPost = `${currentChannel}03`;
    }
    var szUrl = "ws://" + HOST + ":7681/" + curPost;
    console.log("🚀 ~ initLiveView ~ szUrl:", szUrl);
    Plugin.JS_Play(szUrl, {
      sessionID: AUTH,
      token: AUTH
    }, 0);
    /* 插件可能改动 #liveview 尺寸，重新算一次以保持贴合 */
    fitLiveview();
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

  /**
   * Detection box overlay.
   *
   * The application publishes the latest inference result on an ISAPI endpoint.
   * This polls that endpoint and draws the boxes on the canvas stacked between
   * the video and the rule-editing canvas, so the polygon editor keeps working.
   * Polling is asynchronous and self-pacing: the next request is scheduled only
   * after the previous one settles, so a slow device cannot build a backlog.
   */
  function DetectOverlay(id) {
    /* Factory, like RectCanvas: the instance is returned explicitly so callers
       do not depend on `new` or on `this` being the instance. */
    var that = {};

    that.POLL_MS = 250;
    that.STALE_MS = 2000;

    var canvasElement = $("#" + id)[0];
    that.canvasElement = canvasElement;
    that.context2D = canvasElement ? canvasElement.getContext("2d") : null;
    that.width = canvasElement ? canvasElement.width : 0;
    that.height = canvasElement ? canvasElement.height : 0;

    var timer = null;
    var inFlight = false;
    var running = false;
    var enabled = true;
    var lastSeq = -1;
    var lastOkMs = 0;
    var failures = 0;
    var boxes = [];

    function clearCanvas() {
      if (that.context2D) {
        that.context2D.clearRect(0, 0, that.width, that.height);
      }
    }

    function render() {
      var i = 0;

      if (!that.context2D) {
        return;
      }
      clearCanvas();
      if (!enabled) {
        return;
      }

      for (i = 0; i < boxes.length; i++) {
        var box = boxes[i];
        var x = box.x * that.width;
        var y = box.y * that.height;
        var w = box.w * that.width;
        var h = box.h * that.height;
        if (!(w > 0) || !(h > 0)) {
          continue;
        }

        that.context2D.strokeStyle = "#00FF00";
        that.context2D.lineWidth = 2;
        that.context2D.strokeRect(x, y, w, h);

        var szScore = typeof box.confidence === "number" && isFinite(box.confidence)
          ? box.confidence.toFixed(2) : "--";
        var szName = typeof box.name === "string" ? box.name.trim() : "";
        var szLabel = (szName ? szName + " " : "") + "class:" + box.cls + " confidence:" + szScore;
        that.context2D.font = "14px sans-serif";
        var iBaseline = y >= 18 ? y - 4 : y + 16;
        var iTextW = that.context2D.measureText(szLabel).width + 4;
        that.context2D.fillStyle = "rgba(0, 0, 0, 0.5)";
        that.context2D.fillRect(x, iBaseline - 13, iTextW, 15);
        that.context2D.fillStyle = "#00FF00";
        that.context2D.fillText(szLabel, x + 2, iBaseline);
      }
    }

    function takeResult(oJson) {
      lastOkMs = new Date().getTime();
      failures = 0;

      /* 录屏上报地址由设备从 app.conf 的 upload_url 派生后下发，换服务器不用改前
         端。它不依赖 seq，所以要在下面的 seq 判断之前取。 */
      if (oJson && oJson.recordUrl && oJson.recordUrl !== window.CA_RECORD_URL) {
        window.CA_RECORD_URL = oJson.recordUrl;
        console.log("[record] device recordUrl = " + oJson.recordUrl);
      }

      var data = oJson && oJson.cameraAbnormalDetections;
      /* seq 0 means the device has not published a detection yet. */
      if (!data || data.seq === undefined || data.seq === 0) {
        if (boxes.length) {
          boxes = [];
          render();
        }
        return;
      }
      if (data.seq === lastSeq) {
        return; /* no new result, keep the current drawing */
      }
      lastSeq = data.seq;
      boxes = data.boxes && data.boxes.length ? data.boxes : [];
      render();
    }

    function hideStaleBoxes() {
      if (boxes.length && new Date().getTime() - lastOkMs > that.STALE_MS) {
        boxes = [];
        render();
      }
    }

    function schedule() {
      if (running) {
        timer = window.setTimeout(poll, that.POLL_MS);
      }
    }

    function poll() {
      timer = null;
      if (!running) {
        return;
      }
      hideStaleBoxes();
      if (!enabled || inFlight) {
        schedule();
        return;
      }

      var szUrl = "/ISAPI/Custom/OpenPlatform/extern/cameraAbnormal/detections?format=json";
      if (currentChannel > 0) {
        szUrl += "&chanID=" + currentChannel;
      }

      inFlight = true;
      $.ajax({
        url: HTTP + HOST + ":" + PORT + szUrl,
        type: "GET",
        dataType: "json",
        cache: false,
        success: function (oJson) {
          takeResult(oJson);
        },
        error: function () {
          /* Stay silent: a dialog per failed poll would be unusable, and the
             stale timeout already removes the boxes. */
          failures++;
          if (failures === 3) {
            console.warn("detect overlay: detection endpoint unavailable");
          }
          if (failures >= 3 && boxes.length) {
            boxes = [];
            render();
          }
        },
        complete: function () {
          inFlight = false;
          schedule();
        }
      });
    }

    /* Idempotent: initLiveView runs again on every channel switch. */
    that.start = function () {
      if (running) {
        return;
      }
      running = true;
      lastSeq = -1;
      lastOkMs = new Date().getTime();
      poll();
    };

    that.stop = function () {
      running = false;
      if (timer) {
        window.clearTimeout(timer);
        timer = null;
      }
      boxes = [];
      lastSeq = -1;
      clearCanvas();
    };

    /* Drop the current drawing but keep polling: used on channel switch. */
    that.reset = function () {
      boxes = [];
      lastSeq = -1;
      clearCanvas();
    };

    that.setEnabled = function (bEnable) {
      enabled = !!bEnable;
      render();
    };

    that.isEnabled = function () {
      return enabled;
    };

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
  demoInterface.toggleDetect = function (bEnable) {
    detectOverlay.setEnabled(bEnable);
  };
  window.demoInterface = demoInterface;
});

$(function(){
  function rlog(m){ try{ console.log("[record] " + m); }catch(e){} }

  var btn = document.createElement("button");
  btn.id = "btnAutoRecord";
  btn.textContent = "自动录制";
  btn.style.cssText = "position:fixed;right:16px;top:16px;z-index:9999;padding:8px 14px;background:#d71920;color:#fff;border:none;border-radius:2px;cursor:pointer;";
  document.body.appendChild(btn);
  rlog("auto button added");

  /* 兜底地址。实际优先用设备在检测接口里下发的 recordUrl（由 app.conf 的
     upload_url 派生），所以换服务器只改 app.conf，不用改这个文件。 */
  var RECORD_URL = "http://192.168.1.3:8080/api/record";

  function recordUrl(){
    return window.CA_RECORD_URL || RECORD_URL;
  }
  var SEGMENT_MS = 30000;
  var videoCanvas = null, recCanvas = null, recCtx = null, stream = null;
  var recorder = null, chunks = [], autoOn = false, tickTimer = null, segTimer = null;

  function findVideoCanvas(){
    var c = document.querySelector("#liveview canvas");
    if (c) { videoCanvas = c; return true; }
    return false;
  }

  function tick(){
    if (!autoOn || !recCtx) return;
    recCtx.clearRect(0,0,1920,1080);
    if (videoCanvas) recCtx.drawImage(videoCanvas, 0, 0, 1920, 1080);
    var det = document.getElementById("detectCanvas");
    if (det) recCtx.drawImage(det, 0, 0, 1920, 1080);
  }

  function upload(blob){
    var fd = new FormData();
    fd.append("video", blob, "seg_" + Date.now() + ".webm");
    rlog("upload " + blob.size + " bytes -> " + recordUrl());
    fetch(recordUrl(), { method: "POST", body: fd })
      .then(function(r){ rlog("uploaded status=" + r.status); })
      .catch(function(e){ rlog("upload error " + e.message); });
  }

  function startSegment(){
    chunks = [];
    recorder = new MediaRecorder(stream, { mimeType: "video/webm" });
    recorder.ondataavailable = function(e){ if (e.data && e.data.size) chunks.push(e.data); };
    recorder.onstop = function(){
      var blob = new Blob(chunks, { type: "video/webm" });
      if (blob.size > 0) upload(blob);
      if (autoOn) setTimeout(startSegment, 300);
    };
    recorder.start();
    segTimer = setTimeout(function(){ if (recorder && recorder.state !== "inactive") recorder.stop(); }, SEGMENT_MS);
  }

  btn.onclick = function(){
    try {
      if (!autoOn){
        if (!findVideoCanvas()) { alert("未找到视频画面"); return; }
        if (typeof HTMLCanvasElement.prototype.captureStream !== "function") { alert("当前浏览器不支持录制"); return; }
        recCanvas = document.createElement("canvas");
        recCanvas.width = 1920; recCanvas.height = 1080;
        recCtx = recCanvas.getContext("2d");
        stream = recCanvas.captureStream(30);
        autoOn = true;
        btn.textContent = "停止自动录制";
        tickTimer = setInterval(tick, 33);
        startSegment();
        rlog("auto started");
      } else {
        autoOn = false;
        btn.textContent = "自动录制";
        clearInterval(tickTimer);
        if (segTimer) clearTimeout(segTimer);
        if (recorder && recorder.state !== "inactive") recorder.stop();
        rlog("auto stopped");
      }
    } catch(e) {
      alert("录制出错: " + e.message);
      rlog("error: " + e.message);
    }
  };
});
