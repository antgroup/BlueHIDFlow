// BlueHIDFlow Web 配置服务器实现
#include "web_server.h"
#include "connection_manager.h"
#include "config_provider.h"
#include <WiFi.h>
#include <ArduinoJson.h>

// 静态成员定义
const IPAddress ConfigWebServer::AP_IP(192, 168, 8, 1);
const IPAddress ConfigWebServer::AP_GATEWAY(192, 168, 8, 1);
const IPAddress ConfigWebServer::AP_SUBNET(255, 255, 255, 0);

ConfigWebServer webServer;

ConfigWebServer::ConfigWebServer() :
    _server(nullptr),
    _dnsServer(nullptr),
    _running(false),
    _apMode(false),
    _lastClientCount(0),
    _onSaveCallback(nullptr),
    _onRebootCallback(nullptr) {
}

ConfigWebServer::~ConfigWebServer() {
    stop();
}

void ConfigWebServer::begin() {
    if (_running) return;

    _server = new WebServer(80);
    setupRoutes();
    _server->begin();
    _running = true;

    Serial.println("[WebServer] 服务器启动，端口 80");
}

void ConfigWebServer::stop() {
    if (_server) {
        _server->stop();
        delete _server;
        _server = nullptr;
    }
    stopAPMode();
    _running = false;
}

void ConfigWebServer::handleClient() {
    if (_server) _server->handleClient();
    if (_dnsServer) _dnsServer->processNextRequest();

    // 检测 AP 模式下的用户连接状态
    if (_apMode) {
        int currentClients = WiFi.softAPgetStationNum();

        // 检测用户连接
        if (currentClients > 0 && _lastClientCount == 0) {
            // 用户刚连接
            connectionManager.notifyUserConnected();
        }
        // 检测用户断开
        else if (currentClients == 0 && _lastClientCount > 0) {
            connectionManager.notifyUserDisconnected();
        }

        _lastClientCount = currentClients;
    }
}

// ============================================
// AP 模式
// ============================================
void ConfigWebServer::startAPMode() {
    if (_apMode) return;

    Serial.println("[WebServer] 启动 AP 模式...");

    // 断开现有连接并重置 WiFi 模式
    WiFi.disconnect(true, true);
    delay(100);
    WiFi.mode(WIFI_OFF);
    delay(100);
    WiFi.mode(WIFI_AP);
    delay(100);

    // 配置 AP
    if (!WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET)) {
        Serial.println("[WebServer] ❌ softAPConfig 失败!");
    } else {
        Serial.println("[WebServer] ✅ softAPConfig 成功");
    }

    // 启动 AP (channel 6, max 4 connections)
    if (!WiFi.softAP(AP_SSID, AP_PASSWORD, 6, 0, 4)) {
        Serial.println("[WebServer] ❌ softAP 启动失败!");
    } else {
        Serial.println("[WebServer] ✅ softAP 启动成功");
    }

    delay(100);  // 等待 AP 稳定

    // 启动 DNS 服务器 (Captive Portal)
    _dnsServer = new DNSServer();
    if (_dnsServer->start(53, "*", AP_IP)) {
        Serial.println("[WebServer] ✅ DNS 服务器启动成功");
    } else {
        Serial.println("[WebServer] ❌ DNS 服务器启动失败!");
    }

    _apMode = true;

    Serial.printf("[WebServer] AP 已启动: %s\n", AP_SSID);
    Serial.printf("[WebServer] 密码: %s\n", AP_PASSWORD);
    Serial.printf("[WebServer] IP 地址: %s\n", AP_IP.toString().c_str());
    Serial.printf("[WebServer] MAC 地址: %s\n", WiFi.softAPmacAddress().c_str());
}

void ConfigWebServer::stopAPMode() {
    if (!_apMode) return;

    if (_dnsServer) {
        _dnsServer->stop();
        delete _dnsServer;
        _dnsServer = nullptr;
    }

    WiFi.softAPdisconnect(true);
    _apMode = false;

    Serial.println("[WebServer] AP 模式已关闭");
}

// ============================================
// 路由设置
// ============================================
void ConfigWebServer::setupRoutes() {
    // 页面路由
    _server->on("/", std::bind(&ConfigWebServer::handleRoot, this));
    _server->on("/wifi", std::bind(&ConfigWebServer::handleWifi, this));
    _server->on("/mqtt", std::bind(&ConfigWebServer::handleMqtt, this));
    _server->on("/camera", std::bind(&ConfigWebServer::handleCamera, this));
    _server->on("/ai", std::bind(&ConfigWebServer::handleAI, this));
    _server->on("/bluetooth", std::bind(&ConfigWebServer::handleBluetooth, this));
    _server->on("/phone", std::bind(&ConfigWebServer::handlePhone, this));
    _server->on("/system", std::bind(&ConfigWebServer::handleSystem, this));

    // API 路由
    _server->on("/api/config", HTTP_GET, std::bind(&ConfigWebServer::handleApiConfig, this));
    _server->on("/api/config", HTTP_POST, std::bind(&ConfigWebServer::handleApiConfigSave, this));
    _server->on("/api/wifi/scan", std::bind(&ConfigWebServer::handleApiWifiScan, this));
    _server->on("/api/wifi/connect", HTTP_POST, std::bind(&ConfigWebServer::handleApiWifiConnect, this));
    _server->on("/api/mqtt/test", HTTP_POST, std::bind(&ConfigWebServer::handleApiMqttTest, this));
    _server->on("/api/camera/capture", std::bind(&ConfigWebServer::handleApiCameraCapture, this));
    _server->on("/api/status", std::bind(&ConfigWebServer::handleApiStatus, this));
    _server->on("/api/reboot", HTTP_POST, std::bind(&ConfigWebServer::handleApiReboot, this));
    _server->on("/api/reset", HTTP_POST, std::bind(&ConfigWebServer::handleApiReset, this));

    // 静态资源
    _server->onNotFound(std::bind(&ConfigWebServer::handleRoot, this));

    // CORS 支持
    _server->enableCORS(true);
}

// ============================================
// HTML 页面模板
// ============================================
static const char HTML_HEADER[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>BlueHIDFlow 配置</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: #F3F4F6;
            color: #1F2937;
            line-height: 1.5;
            padding: 16px;
        }
        .container { max-width: 480px; margin: 0 auto; }
        .header {
            background: linear-gradient(135deg, #3B82F6, #2563EB);
            color: white;
            padding: 20px;
            border-radius: 12px;
            margin-bottom: 16px;
            text-align: center;
        }
        .header h1 { font-size: 24px; font-weight: 600; }
        .header p { font-size: 14px; opacity: 0.9; margin-top: 4px; }
        .card {
            background: white;
            border-radius: 12px;
            padding: 20px;
            margin-bottom: 16px;
            box-shadow: 0 1px 3px rgba(0,0,0,0.1);
        }
        .card-title {
            font-size: 16px; font-weight: 600;
            display: flex; align-items: center; gap: 8px;
            margin-bottom: 16px; padding-bottom: 12px;
            border-bottom: 1px solid #E5E7EB;
        }
        .form-group { margin-bottom: 16px; }
        .form-group label {
            display: block; font-size: 14px; font-weight: 500;
            color: #374151; margin-bottom: 6px;
        }
        .form-group input, .form-group select {
            width: 100%; padding: 10px 12px;
            border: 1px solid #D1D5DB; border-radius: 8px;
            font-size: 14px; transition: border-color 0.2s;
        }
        .form-group input:focus, .form-group select:focus {
            outline: none; border-color: #3B82F6;
            box-shadow: 0 0 0 3px rgba(59, 130, 246, 0.1);
        }
        .btn {
            display: inline-flex; align-items: center; justify-content: center;
            padding: 10px 16px; border-radius: 8px; font-size: 14px;
            font-weight: 500; cursor: pointer; border: none; transition: all 0.2s;
        }
        .btn-primary { background: #3B82F6; color: white; }
        .btn-primary:hover { background: #2563EB; }
        .btn-secondary { background: #E5E7EB; color: #374151; }
        .btn-secondary:hover { background: #D1D5DB; }
        .btn-danger { background: #EF4444; color: white; }
        .btn-danger:hover { background: #DC2626; }
        .btn-block { width: 100%; }
        .btn-group { display: flex; gap: 8px; margin-top: 16px; }
        .status { padding: 8px 12px; border-radius: 6px; font-size: 13px; margin-top: 8px; }
        .status-success { background: #D1FAE5; color: #065F46; }
        .status-error { background: #FEE2E2; color: #991B1B; }
        .nav { display: flex; flex-wrap: wrap; gap: 8px; margin-bottom: 16px; }
        .nav a {
            flex: 1; min-width: 100px; text-align: center;
            padding: 10px 12px; background: white;
            border-radius: 8px; text-decoration: none;
            color: #374151; font-size: 13px; font-weight: 500;
            box-shadow: 0 1px 2px rgba(0,0,0,0.05);
        }
        .nav a:hover, .nav a.active { background: #3B82F6; color: white; }
        .loading { opacity: 0.6; pointer-events: none; }
        .hint { font-size: 12px; color: #6B7280; margin-top: 4px; }
        .wifi-list { margin-top: 8px; }
        .wifi-item {
            display: flex; align-items: center; justify-content: space-between;
            padding: 10px 12px; background: #F9FAFB; border-radius: 6px;
            margin-bottom: 6px; cursor: pointer;
        }
        .wifi-item:hover { background: #F3F4F6; }
        .wifi-item .ssid { font-weight: 500; }
        .wifi-item .rssi { font-size: 12px; color: #6B7280; }
    </style>
</head>
<body>
<div class="container">
    <div class="header">
        <h1>🔧 BlueHIDFlow</h1>
        <p>设备配置管理</p>
    </div>
    <div class="nav">
        <a href="/" id="nav-home">首页</a>
        <a href="/wifi" id="nav-wifi">WiFi</a>
        <a href="/mqtt" id="nav-mqtt">MQTT</a>
        <a href="/camera" id="nav-camera">摄像头</a>
        <a href="/ai" id="nav-ai">大模型</a>
        <a href="/bluetooth" id="nav-bt">蓝牙</a>
        <a href="/phone" id="nav-phone">手机</a>
    </div>
)rawliteral";

static const char HTML_FOOTER[] PROGMEM = R"rawliteral(
</div>
<script>
function $(id) { return document.getElementById(id); }
function getJson(url, cb) {
    fetch(url).then(r => r.json()).then(cb).catch(e => alert('请求失败: ' + e));
}
function postJson(url, data, cb) {
    fetch(url, {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(data)
    }).then(r => r.json()).then(cb).catch(e => alert('请求失败: ' + e));
}
function setStatus(el, success, msg) {
    el.className = 'status ' + (success ? 'status-success' : 'status-error');
    el.textContent = msg;
}
</script>
</body>
</html>
)rawliteral";

// ============================================
// 页面路由实现
// ============================================
void ConfigWebServer::handleRoot() {
    String html = String(HTML_HEADER);

    // 获取设备 ID：优先使用 MQTT deviceId（用户设置），如果为空则自动生成（基于 MAC 地址或时间戳）
    String deviceId = configManager.mqtt().deviceId;
    if (deviceId.length() == 0) {
        deviceId = configManager.generateDeviceId();
    }

    // 获取配置验证错误
    String validationErrors = configManager.getValidationErrors();

    html += R"rawliteral(
    <div class="card">
        <div class="card-title">📊 设备状态</div>
        <div class="form-group">
            <label>设备 ID</label>
            <input type="text" id="device_id" value=")rawliteral";
    html += deviceId;
    html += R"rawliteral(">
            <p class="hint">设备唯一标识，可自定义修改</p>
        </div>
        <div class="form-group">
            <label>配置状态</label>
            <input type="text" readonly value=")rawliteral";
    html += configManager.isComplete() ? "✅ 配置完整" : "⚠️ 配置不完整";
    html += R"rawliteral(" style="background:#f9fafb;">
        </div>
    </div>
    <div class="card">
        <div class="card-title">📋 配置检查</div>
        <div style="font-size:13px;">
)rawliteral";

    // 显示各项配置状态
    html += "<div style='margin-bottom:8px;'>";
    html += configManager.wifi().isValid() ? "✅" : "❌";
    html += " WiFi 配置";
    if (!configManager.wifi().isValid()) html += " <span style='color:#EF4444;'>(未配置)</span>";
    html += "</div>";

    html += "<div style='margin-bottom:8px;'>";
    html += configManager.mqtt().isValid() ? "✅" : "❌";
    html += " MQTT 配置";
    if (!configManager.mqtt().isValid()) html += " <span style='color:#EF4444;'>(broker或deviceId缺失)</span>";
    html += "</div>";

    html += "<div style='margin-bottom:8px;'>";
    html += configManager.ai().isValid() ? "✅" : "❌";
    html += " 大模型配置";
    if (!configManager.ai().isValid()) html += " <span style='color:#EF4444;'>(API Key缺失)</span>";
    html += "</div>";

    html += "<div style='margin-bottom:8px;'>";
    html += configManager.phone().isValid() ? "✅" : "✅";
    html += " 手机配置 (可选)";
    html += "</div>";

    // 显示验证错误
    if (validationErrors.length() > 0) {
        html += R"rawliteral(
        </div>
        <div style="background:#FEF2F2;border:1px solid #FECACA;border-radius:6px;padding:10px;margin-top:12px;">
            <div style="color:#991B1B;font-weight:500;margin-bottom:4px;">⚠️ 配置错误</div>
            <div style="color:#7F1D1D;font-size:12px;white-space:pre-line;">)rawliteral";
        html += validationErrors;
        html += R"rawliteral(</div>
        </div>
)rawliteral";
    }

    html += R"rawliteral(
    </div>
    <div class="card">
        <div class="card-title">⚡ 快速操作</div>
        <div class="btn-group">
            <button class="btn btn-primary btn-block" onclick="location.href='/wifi'">配置 WiFi</button>
        </div>
        <div class="btn-group">
            <button class="btn btn-secondary" onclick="location.href='/mqtt'">配置 MQTT</button>
            <button class="btn btn-secondary" onclick="location.href='/ai'">配置大模型</button>
        </div>
    </div>
    <div class="card">
        <div class="card-title">🔧 完成配置</div>
        <p class="hint" style="margin-bottom:12px;">完成所有必要配置后，点击下方按钮保存并重启设备。</p>
        <div class="btn-group">
            <button class="btn btn-success btn-block" style="background:#10B981;" onclick="saveDeviceIdAndReboot()" id="btn-complete">💾 保存配置并重启</button>
        </div>
        <div id="status" class="status" style="display:none;margin-top:12px;"></div>
    </div>
    <script>
    document.getElementById('nav-home').classList.add('active');

    function saveDeviceIdAndReboot() {
        var btn = $('btn-complete');
        btn.disabled = true;
        btn.textContent = '保存中...';

        var deviceId = $('device_id').value.trim();

        // 保存设备ID到蓝牙和MQTT配置
        postJson('/api/config', {
            bluetooth: { deviceName: deviceId },
            mqtt: { deviceId: deviceId }
        }, function(r) {
            if (r.success) {
                var s = $('status');
                s.style.display = 'block';
                s.className = 'status status-success';
                s.textContent = '✅ 配置已保存，设备即将重启...';

                setTimeout(function() {
                    postJson('/api/reboot', {}, function() {});
                }, 1000);
            } else {
                var s = $('status');
                s.style.display = 'block';
                s.className = 'status status-error';
                s.textContent = '❌ 保存失败: ' + (r.error || '未知错误');
                btn.disabled = false;
                btn.textContent = '💾 保存配置并重启';
            }
        });
    }
    </script>
)rawliteral";
    html += HTML_FOOTER;
    _server->send(200, "text/html; charset=utf-8", html);
}

void ConfigWebServer::handleWifi() {
    String html = String(HTML_HEADER);
    html += R"rawliteral(
    <div class="card">
        <div class="card-title">📶 WiFi 设置</div>
        <div class="form-group">
            <label>网络名称 (SSID)</label>
            <input type="text" id="ssid" placeholder="输入或扫描网络" value=")rawliteral";
    html += configManager.wifi().ssid;
    html += R"rawliteral(">
        </div>
        <div class="form-group">
            <label>密码</label>
            <input type="password" id="password" placeholder="输入 WiFi 密码（留空则保留原密码）" value="">
        </div>
        <div class="btn-group">
            <button class="btn btn-secondary" onclick="scanWifi()">扫描网络</button>
            <button class="btn btn-primary" onclick="saveWifi()">保存</button>
        </div>
        <div id="wifi-list" class="wifi-list"></div>
        <div id="status" class="status" style="display:none;"></div>
    </div>
    <script>
    document.getElementById('nav-wifi').classList.add('active');
    function scanWifi() {
        getJson('/api/wifi/scan', function(data) {
            var html = '';
            data.networks.forEach(function(n) {
                html += '<div class="wifi-item" onclick="selectWifi(\''+n.ssid+'\')">';
                html += '<span class="ssid">'+n.ssid+'</span>';
                html += '<span class="rssi">'+n.rssi+' dBm</span></div>';
            });
            $('wifi-list').innerHTML = html || '<p class="hint">未找到网络</p>';
        });
    }
    function selectWifi(ssid) { $('ssid').value = ssid; }
    function saveWifi() {
        var wifiCfg = {ssid:$('ssid').value};
        if ($('password').value.length > 0) {
            wifiCfg.password = $('password').value;
        }
        postJson('/api/config', {wifi: wifiCfg}, function(r) {
            var s = $('status'); s.style.display = 'block';
            setStatus(s, r.success, r.success ? '✅ 保存成功' : '❌ ' + r.error);
        });
    }
    </script>
)rawliteral";
    html += HTML_FOOTER;
    _server->send(200, "text/html; charset=utf-8", html);
}

void ConfigWebServer::handleMqtt() {
    String html = String(HTML_HEADER);
    html += R"rawliteral(
    <div class="card">
        <div class="card-title">📡 MQTT 设置</div>
        <div class="form-group">
            <label>Broker 地址</label>
            <input type="text" id="broker" placeholder="例如: your-broker.example.com" value=")rawliteral";
    html += configManager.mqtt().broker;
    html += R"rawliteral(">
        </div>
        <div class="form-group">
            <label>端口</label>
            <input type="number" id="port" value=")rawliteral";
    html += String(configManager.mqtt().port);
    html += R"rawliteral(">
        </div>
        <div class="form-group">
            <label>用户名 (可选)</label>
            <input type="text" id="username" value=")rawliteral";
    html += configManager.mqtt().username;
    html += R"rawliteral(">
        </div>
        <div class="form-group">
            <label>密码 (可选)</label>
            <input type="password" id="mqtt_password" placeholder="MQTT 密码（留空则保留原密码）" value="">
        </div>
        <div class="form-group">
            <label>TLS CA 证书 (可选)</label>
            <textarea id="mqtt_ca" rows="8" placeholder="-----BEGIN CERTIFICATE-----\n...\n-----END CERTIFICATE-----">)rawliteral";
    html += configManager.mqtt().caCert;
    html += R"rawliteral(</textarea>
            <p class="hint">MQTT TLS 连接所需 CA 证书，不配置时尝试使用 config.h 中的 MQTT_CA_CERT</p>
        </div>
        <div class="form-group">
            <label>设备 ID</label>
            <input type="text" id="device_id" value=")rawliteral";
    // 获取 deviceId，如果为空则使用自动生成的值
    String deviceId = configManager.mqtt().deviceId;
    if (deviceId.length() == 0) {
        deviceId = configManager.generateDeviceId();
    }
    html += deviceId;
    html += R"rawliteral(">
            <p class="hint">默认使用 MAC 地址，可自定义修改</p>
        </div>
        <div class="btn-group">
            <button class="btn btn-primary btn-block" onclick="saveMqtt()">保存配置</button>
        </div>
        <div id="status" class="status" style="display:none;"></div>
    </div>
    <script>
    document.getElementById('nav-mqtt').classList.add('active');
    function saveMqtt() {
        var mqttCfg = {
            broker: $('broker').value,
            port: parseInt($('port').value) || 8883,
            username: $('username').value,
            caCert: $('mqtt_ca').value,
            deviceId: $('device_id').value
        };
        if ($('mqtt_password').value.length > 0) {
            mqttCfg.password = $('mqtt_password').value;
        }
        postJson('/api/config', {mqtt: mqttCfg}, function(r) {
            var s = $('status'); s.style.display = 'block';
            setStatus(s, r.success, r.success ? '✅ 保存成功' : '❌ ' + r.error);
        });
    }
    </script>
)rawliteral";
    html += HTML_FOOTER;
    _server->send(200, "text/html; charset=utf-8", html);
}

void ConfigWebServer::handleCamera() {
    String html = String(HTML_HEADER);
    html += R"rawliteral(
    <div class="card">
        <div class="card-title">📷 摄像头设置</div>
        <div class="form-group">
            <label>分辨率</label>
            <select id="resolution">
                <option value="0")rawliteral";
    if (configManager.camera().resolution == 0) html += " selected";
    html += R"rawliteral(>QVGA (320×240) - 最小</option>
                <option value="1")rawliteral";
    if (configManager.camera().resolution == 1) html += " selected";
    html += R"rawliteral(>VGA (640×480)</option>
                <option value="2")rawliteral";
    if (configManager.camera().resolution == 2) html += " selected";
    html += R"rawliteral(>SVGA (800×600) - 推荐</option>
                <option value="3")rawliteral";
    if (configManager.camera().resolution == 3) html += " selected";
    html += R"rawliteral(>XGA (1024×768)</option>
                <option value="4")rawliteral";
    if (configManager.camera().resolution == 4) html += " selected";
    html += R"rawliteral(>SXGA (1280×960) - 最大</option>
            </select>
        </div>
        <div class="form-group">
            <label>JPEG 质量 (1-63，越小质量越高)</label>
            <input type="number" id="quality" min="1" max="63" value=")rawliteral";
    html += String(configManager.camera().jpegQuality);
    html += R"rawliteral(">
        </div>
        <div class="form-group">
            <label>摄像头距离 (cm)</label>
            <input type="number" id="distance" min="5" max="50" value=")rawliteral";
    html += String(configManager.camera().distanceCm);
    html += R"rawliteral(">
            <p class="hint">推荐 15-18cm</p>
        </div>
        <div class="btn-group">
            <button class="btn btn-primary btn-block" onclick="saveCamera()">保存配置</button>
        </div>
        <div id="status" class="status" style="display:none;"></div>
    </div>
    <script>
    document.getElementById('nav-camera').classList.add('active');
    function saveCamera() {
        postJson('/api/config', {
            camera: {
                resolution: parseInt($('resolution').value),
                jpegQuality: parseInt($('quality').value),
                distanceCm: parseInt($('distance').value)
            }
        }, function(r) {
            var s = $('status'); s.style.display = 'block';
            setStatus(s, r.success, r.success ? '✅ 保存成功' : '❌ ' + r.error);
        });
    }
    </script>
)rawliteral";
    html += HTML_FOOTER;
    _server->send(200, "text/html; charset=utf-8", html);
}

void ConfigWebServer::handleAI() {
    String html = String(HTML_HEADER);
    html += R"rawliteral(
    <div class="card">
        <div class="card-title">🤖 大模型设置</div>
        <div class="form-group">
            <label>API Key</label>
            <input type="password" id="api_key" placeholder="通义千问 API Key（留空则保留原 Key）" value="">
        </div>
        <div class="form-group">
            <label>API URL</label>
            <input type="text" id="api_url" value=")rawliteral";
    html += configManager.ai().apiUrl;
    html += R"rawliteral(">
        </div>
        <div class="form-group">
            <label>模型名称</label>
            <input type="text" id="model" value=")rawliteral";
    html += configManager.ai().model;
    html += R"rawliteral(">
        </div>
        <div class="form-group">
            <label>超时时间 (ms)</label>
            <input type="number" id="timeout" value=")rawliteral";
    html += String(configManager.ai().timeoutMs);
    html += R"rawliteral(">
        </div>
        <div class="btn-group">
            <button class="btn btn-primary btn-block" onclick="saveAI()">保存配置</button>
        </div>
        <div id="status" class="status" style="display:none;"></div>
    </div>
    <script>
    document.getElementById('nav-ai').classList.add('active');
    function saveAI() {
        var aiCfg = {
            apiUrl: $('api_url').value,
            model: $('model').value,
            timeoutMs: parseInt($('timeout').value)
        };
        if ($('api_key').value.length > 0) {
            aiCfg.apiKey = $('api_key').value;
        }
        postJson('/api/config', {ai: aiCfg}, function(r) {
            var s = $('status'); s.style.display = 'block';
            setStatus(s, r.success, r.success ? '✅ 保存成功' : '❌ ' + r.error);
        });
    }
    </script>
)rawliteral";
    html += HTML_FOOTER;
    _server->send(200, "text/html; charset=utf-8", html);
}

void ConfigWebServer::handleBluetooth() {
    String html = String(HTML_HEADER);
    html += R"rawliteral(
    <div class="card">
        <div class="card-title">📱 蓝牙设置</div>
        <div class="form-group">
            <label>设备名称</label>
            <input type="text" id="bt_name" value=")rawliteral";
    html += configManager.bluetooth().deviceName;
    html += R"rawliteral(">
            <p class="hint">留空则自动生成</p>
        </div>
        <div class="form-group">
            <label>HID 模式</label>
            <select id="hid_mode">
                <option value="0")rawliteral";
    if (configManager.bluetooth().hidMode == HIDMode::KEYBOARD) html += " selected";
    html += R"rawliteral(>键盘模式 (文字输入、系统键、媒体键)</option>
                <option value="1")rawliteral";
    if (configManager.bluetooth().hidMode == HIDMode::MOUSE) html += " selected";
    html += R"rawliteral(>鼠标模式 (精确定位点击)</option>
            </select>
            <p class="hint">切换模式需要重启设备才能生效</p>
        </div>
        <div class="btn-group">
            <button class="btn btn-primary btn-block" onclick="saveBT()">保存配置</button>
        </div>
        <div id="status" class="status" style="display:none;"></div>
    </div>
    <script>
    document.getElementById('nav-bt').classList.add('active');
    function saveBT() {
        postJson('/api/config', {
            bluetooth: {
                deviceName: $('bt_name').value,
                hidMode: parseInt($('hid_mode').value)
            }
        }, function(r) {
            var s = $('status'); s.style.display = 'block';
            setStatus(s, r.success, r.success ? '✅ 保存成功' : '❌ ' + r.error);
        });
    }
    </script>
)rawliteral";
    html += HTML_FOOTER;
    _server->send(200, "text/html; charset=utf-8", html);
}

void ConfigWebServer::handlePhone() {
    String html = String(HTML_HEADER);

    html += R"rawliteral(
    <div class="card">
        <div class="card-title">📱 手机设置</div>
        <div class="form-group">
            <label>手机型号</label>
            <input type="text" id="phone_model" placeholder="例如：HUAWEI P60" value=")rawliteral";
    html += configManager.phone().model;
    html += R"rawliteral(">
        </div>
        <div class="form-group">
            <label>屏幕宽度 (像素)</label>
            <input type="number" id="screen_w" value=")rawliteral";
    html += String(configManager.phone().screenWidth);
    html += R"rawliteral(">
        </div>
        <div class="form-group">
            <label>屏幕高度 (像素)</label>
            <input type="number" id="screen_h" value=")rawliteral";
    html += String(configManager.phone().screenHeight);
    html += R"rawliteral(">
        </div>
        <div class="form-group">
            <label>HID 灵敏度 (像素/HID 单位)</label>
            <input type="number" id="hid_sens" step="0.01" min="0.1" max="10" value=")rawliteral";
    html += String(configManager.phone().hidSensitivity, 2);
    html += R"rawliteral(">
            <p class="hint">1 HID 单位移动的像素数（仅鼠标模式有效）。<br>
            华为 P60 测试值：1.0<br>
            REDMI K70 测试值：1.53<br>
            如果点击位置偏左上，增大此值；偏右下，减小此值。</p>
        </div>
        <p class="hint">常见手机分辨率：<br>
        iPhone 15 Pro: 2556×1179<br>
        iPhone 15 Pro Max: 2796×1290<br>
        Samsung S24: 2340×1080<br>
        Xiaomi 14: 2670×1200<br>
        HUAWEI P60: 2700×1220</p>
        <div class="btn-group">
            <button class="btn btn-primary btn-block" onclick="savePhone()">保存配置</button>
        </div>
        <div id="status" class="status" style="display:none;"></div>
    </div>
    <script>
    document.getElementById('nav-phone').classList.add('active');
    function savePhone() {
        postJson('/api/config', {
            phone: {
                model: $('phone_model').value,
                screenWidth: parseInt($('screen_w').value),
                screenHeight: parseInt($('screen_h').value),
                hidSensitivity: parseFloat($('hid_sens').value)
            }
        }, function(r) {
            var s = $('status'); s.style.display = 'block';
            setStatus(s, r.success, r.success ? '✅ 保存成功' : '❌ ' + r.error);
        });
    }
    </script>
)rawliteral";
    html += HTML_FOOTER;
    _server->send(200, "text/html; charset=utf-8", html);
}

void ConfigWebServer::handleSystem() {
    String html = String(HTML_HEADER);
    html += R"rawliteral(
    <div class="card">
        <div class="card-title">⚙️ 系统设置</div>
        <div class="form-group">
            <label>日志级别</label>
            <select id="log_level">
                <option value="0")rawliteral";
    if (configManager.system().logLevel == LogLevel::DEBUG) html += " selected";
    html += R"rawliteral(>DEBUG</option>
                <option value="1")rawliteral";
    if (configManager.system().logLevel == LogLevel::INFO) html += " selected";
    html += R"rawliteral(>INFO</option>
                <option value="2")rawliteral";
    if (configManager.system().logLevel == LogLevel::WARN) html += " selected";
    html += R"rawliteral(>WARN</option>
                <option value="3")rawliteral";
    if (configManager.system().logLevel == LogLevel::ERROR) html += " selected";
    html += R"rawliteral(>ERROR</option>
            </select>
        </div>
        <div class="btn-group">
            <button class="btn btn-danger btn-block" onclick="if(confirm('确定要重置所有配置吗？'))resetConfig()">重置所有配置</button>
        </div>
        <div class="btn-group">
            <button class="btn btn-danger btn-block" onclick="if(confirm('确定要重启设备吗？'))reboot()">重启设备</button>
        </div>
        <div id="status" class="status" style="display:none;"></div>
    </div>
    <script>
    function resetConfig() {
        postJson('/api/reset', {}, function(r) {
            var s = $('status'); s.style.display = 'block';
            setStatus(s, r.success, r.success ? '✅ 重置成功，设备将重启...' : '❌ ' + r.error);
        });
    }
    function reboot() {
        postJson('/api/reboot', {}, function(r) {
            var s = $('status'); s.style.display = 'block';
            setStatus(s, true, '✅ 设备正在重启...');
        });
    }
    </script>
)rawliteral";
    html += HTML_FOOTER;
    _server->send(200, "text/html; charset=utf-8", html);
}

// ============================================
// API 路由实现
// ============================================
void ConfigWebServer::handleApiConfig() {
    // 使用 ConfigProviderRegistry 统一序列化
    String output = ConfigProviderRegistry::instance().allToJson();
    sendJSON(200, output);
}

void ConfigWebServer::handleApiConfigSave() {
    String body = getPostBody();
    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, body);
    if (error) {
        sendError(400, "JSON 解析失败");
        return;
    }

    // 使用 ConfigProviderRegistry 统一反序列化
    auto& registry = ConfigProviderRegistry::instance();
    const char* sections[] = {"wifi", "camera", "mqtt", "ai", "bluetooth", "phone", "system"};
    bool anyApplied = false;

    for (const char* sec : sections) {
        if (doc[sec].is<JsonObject>()) {
            IConfigProvider* provider = registry.find(sec);
            if (provider) {
                provider->fromJson(doc[sec].as<JsonObject>());
                anyApplied = true;
            }
        }
    }

    if (!anyApplied) {
        sendError(400, "没有有效的配置节");
        return;
    }

    // 保存配置
    if (!configManager.save()) {
        sendError(500, "保存配置失败");
        return;
    }

    if (_onSaveCallback) _onSaveCallback();

    sendSuccess("配置已保存");
}

void ConfigWebServer::handleApiWifiScan() {
    Serial.println("[WebServer] 扫描 WiFi...");

    int n = WiFi.scanNetworks();
    JsonDocument doc;
    JsonArray networks = doc["networks"].to<JsonArray>();

    for (int i = 0; i < n && i < 20; i++) {
        JsonObject net = networks.add<JsonObject>();
        net["ssid"] = WiFi.SSID(i);
        net["rssi"] = WiFi.RSSI(i);
        net["open"] = WiFi.encryptionType(i) == WIFI_AUTH_OPEN;
    }

    String output;
    serializeJson(doc, output);
    sendJSON(200, output);
}

void ConfigWebServer::handleApiWifiConnect() {
    String body = getPostBody();
    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, body);
    if (error) {
        sendError(400, "JSON 解析失败");
        return;
    }

    String ssid = doc["ssid"] | "";
    String password = doc["password"] | "";

    if (ssid.length() == 0) {
        sendError(400, "SSID 不能为空");
        return;
    }

    // 保存配置
    configManager.wifi().ssid = ssid;
    configManager.wifi().password = password;
    configManager.save();

    sendSuccess("WiFi 配置已保存，请重启设备连接");
}

void ConfigWebServer::handleApiMqttTest() {
    // TODO: 实现 MQTT 连接测试
    sendSuccess("MQTT 测试功能待实现");
}

void ConfigWebServer::handleApiCameraCapture() {
    // TODO: 实现摄像头拍照测试
    sendError(501, "摄像头测试功能待实现");
}

void ConfigWebServer::handleApiStatus() {
    JsonDocument doc;

    doc["deviceId"] = configManager.mqtt().deviceId.length() > 0 ?
                     configManager.mqtt().deviceId : configManager.generateDeviceId();
    doc["configComplete"] = configManager.isComplete();
    doc["wifiConnected"] = WiFi.isConnected();
    doc["wifiSSID"] = WiFi.SSID();
    doc["wifiRSSI"] = WiFi.RSSI();
    doc["ip"] = WiFi.localIP().toString();
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["uptime"] = millis() / 1000;

    String output;
    serializeJson(doc, output);
    sendJSON(200, output);
}

void ConfigWebServer::handleApiReboot() {
    sendSuccess("设备正在重启...");
    delay(100);
    ESP.restart();
}

void ConfigWebServer::handleApiReset() {
    configManager.reset();
    sendSuccess("配置已重置，设备将重启...");
    delay(100);
    ESP.restart();
}

// ============================================
// 工具方法
// ============================================
void ConfigWebServer::sendJSON(int code, const String& json) {
    _server->send(code, "application/json; charset=utf-8", json);
}

void ConfigWebServer::sendError(int code, const String& message) {
    JsonDocument doc;
    doc["success"] = false;
    doc["error"] = message;
    String output;
    serializeJson(doc, output);
    sendJSON(code, output);
}

void ConfigWebServer::sendSuccess(const String& message) {
    JsonDocument doc;
    doc["success"] = true;
    doc["message"] = message;
    String output;
    serializeJson(doc, output);
    sendJSON(200, output);
}

String ConfigWebServer::getPostBody() {
    if (!_server->hasArg("plain")) return "";
    return _server->arg("plain");
}

String ConfigWebServer::escapeJSON(const String& input) {
    String output;
    output.reserve(input.length() * 2);
    for (size_t i = 0; i < input.length(); i++) {
        char c = input[i];
        switch (c) {
            case '"': output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\n': output += "\\n"; break;
            case '\r': output += "\\r"; break;
            case '\t': output += "\\t"; break;
            default: output += c;
        }
    }
    return output;
}