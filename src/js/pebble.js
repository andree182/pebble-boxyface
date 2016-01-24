// the config dict is sent as app message to the watch

var config = {
	cfgLock:           1,

    colorBgBt:         BOXYFACE_KEY_BG_BT_COLOR,
	colorBgNoBt:       BOXYFACE_KEY_BG_NOBT_COLOR,
	colorText:         BOXYFACE_KEY_TEXT_COLOR,
	colorBgText:       BOXYFACE_KEY_TEXT_BG_COLOR

	hourLeadingZero:   1,
	showBatteryStatus: 1,
};

var send_in_progress = false;

// config.html will be included by build process, see build/src/js/pebble-js-app.js
var config_html; 

function send_config_to_pebble_lock(lock) {
    if (send_in_progress) {
        return console.log("cannot send config, already in progress");
    }
    send_in_progress = true;
	config.lock = lock;
    console.log("sending config " + JSON.stringify(config)); 
    window.localStorage.setItem('pebbleNeedsConfig', 'true');
    Pebble.sendAppMessage(config,
        function ack(e) {
            console.log("Successfully delivered message " + JSON.stringify(e.data));
            if (lock)
                send_config_to_pebble_lock(false);
            else
                send_in_progress = false;
            window.localStorage.removeItem('pebbleNeedsConfig');
        },
        function nack(e) {
            console.log("Unable to deliver message " + JSON.stringify(e));
            send_in_progress = false;
        });
}

function send_config_to_pebble() {
	/* not optimal, but works, so... */
	send_config_to_pebble_lock(true);
}

// read config from persistent storage
Pebble.addEventListener('ready',
    function () {
        var json = window.localStorage.getItem('config');
        if (typeof json === 'string') {
            config = JSON.parse(json);
            console.log("loaded config " + JSON.stringify(config));
        }
        if (window.localStorage.getItem('pebbleNeedsConfig')) {
            send_config_to_pebble();
        }
    });

// got message from pebble
Pebble.addEventListener("appmessage",
    function(e) {
        console.log("got message " + JSON.stringify(e.payload));
        if (e.payload.request_config) {
            send_config_to_pebble();
        }
    });

// open config window
Pebble.addEventListener('showConfiguration',
    function () {
        console.log("show config window " + JSON.stringify(config));
        var html = config_html.replace('"REPLACE_ME_AT_RUNTIME"', JSON.stringify(config), 'g');
        Pebble.openURL('data:text/html,' + encodeURIComponent(html + '<!--.html'));
    });

// store config and send to watch
Pebble.addEventListener('webviewclosed',
    function (e) {
        if (e.response && e.response.length) {
            var json = decodeURIComponent(e.response);
            config = JSON.parse(json);
            console.log("storing config " + json);
            window.localStorage.setItem('config', json);
            send_config_to_pebble();
        }
    });

