<?php
    function print_setting($setting)
    {
        $cmd = "/opt/zedboard/bin/hsconf print ".$setting;
        $output = shell_exec($cmd);
        return $output;
    }

    function set_setting($setting)
    {
        $cmd = "/opt/zedboard/bin/hsconf set ".$setting;
        $output = shell_exec($cmd);
        return $output;
    }

    if ($_SERVER['REQUEST_METHOD'] == "POST"){
        $var = $_POST['ssid'];
        if(!is_null($var))
            set_setting("ssid=".$var);
        $var = $_POST['wifip'];
        if(!is_null($var))
            set_setting("wifip=".$var);
        $var = $_POST['satid'];
        if(!is_null($var))
            set_setting("satid=".$var);
        $var = $_POST['ble'];
        if(!is_null($var))
            set_setting("ble=".$var);
        $var = $_POST['gpslat'];
        if(!is_null($var))
            set_setting("gpslat=".$var);
        $var = $_POST['gpslong'];
        if(!is_null($var))
            set_setting("gpslong=".$var);
        $var = $_POST['gpsalt'];
        if(!is_null($var))
            set_setting("gpsalt=".$var);
        $var = $_POST['servid'];
        if(!is_null($var))
            set_setting("servid=".$var);
        $var = $_POST['fms'];
        if(!is_null($var))
            set_setting("fms=".$var);
    }
?>


<!DOCTYPE html>
<html lang="en">
  <head>
    <link rel="stylesheet" href="./style.css" />
    <link
      href="https://fonts.googleapis.com/css?family=Poppins|Roboto&display=swap"
      rel="stylesheet"
    />
    <meta charset="UTF-8" />
    <meta http-equiv="X-UA-Compatible" content="IE=edge" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>HUBLocalLogger</title>
  </head>
  <body>
    <div class="app">
      <header>
        <img alt="hisky-logo" src="./logo.png" class="imageLogo" />
      </header>
      <div class="mainWrapper">
        <div class="sideBar">
          <a href="?print_setting=ssid" class="link">WiFi SSID</a>
          <a href="?print_setting=wifip" class="link">WiFi Password</a>
          <a href="?print_setting=satid" class="link">Satellite Index</a>
          <a href="?print_setting=ble" class="link">Bluetooth MAC Address</a>
          <a href="?print_setting=gpslat" class="link">GPS Latitude</a>
          <a href="?print_setting=gpslong" class="link">GPS Longitude</a>
          <a href="?print_setting=gpsalt" class="link">GPS Altitude</a>
          <a href="?print_setting=servid" class="link">Service ID</a>
          <a href="?print_setting=fms" class="link">FMS Interval</a>
        </div>
        <main>
          <h4>Configuration Settings</h4>
          <div class="breakline"></div>

          <?php
            if (isset($_GET['print_setting'])) {
                if ($_GET['print_setting'] == "ssid"){
                    $setting_value=print_setting($_GET['print_setting']);
                    $setting_selected = "SSID";
                    $setting_id = "ssid";
                }
                else if ($_GET['print_setting'] == "wifip"){
                    $setting_value=print_setting($_GET['print_setting']);
                    $setting_selected = "WiFi Password";
                    $setting_id = "wifip";
                }
                else if ($_GET['print_setting'] == "satid"){
                    $setting_value=print_setting($_GET['print_setting']);
                    $setting_selected = "Satellite ID";
                    $setting_id = "satid";
                }
                else if ($_GET['print_setting'] == "ble"){
                    $setting_value=print_setting($_GET['print_setting']);
                    $setting_selected = "Bluetooth MAC Address";
                    $setting_id = "ble";
                }
                else if ($_GET['print_setting'] == "gpslat"){
                    $setting_value=print_setting($_GET['print_setting']);
                    $setting_selected = "GPS Latitude";
                    $setting_id = "gpslat";
                }
                else if ($_GET['print_setting'] == "gpslong"){
                    $setting_value=print_setting($_GET['print_setting']);
                    $setting_selected = "GPS Longitude";
                    $setting_id = "gpslong";
                }
                else if ($_GET['print_setting'] == "gpsalt"){
                    $setting_value=print_setting($_GET['print_setting']);
                    $setting_selected = "GPS ALtitude";
                    $setting_id = "gpsalt";
                }
                else if ($_GET['print_setting'] == "servid"){
                    $setting_value=print_setting($_GET['print_setting']);
                    $setting_selected = "Service ID";
                    $setting_id = "servid";
                }
                else if ($_GET['print_setting'] == "fms"){
                    $setting_value=print_setting($_GET['print_setting']);
                    $setting_selected = "FMS Interval";
                    $setting_id = "fms";
                }
                else {
                    $setting_value = "Not Available";
                }
            }
            ?>
            <form method="POST">
            <label for="settingID"><?php echo $setting_selected; ?> :</label>
            <input type="text" name=<?php echo $setting_id; ?> id=<?php echo $setting_id; ?> value="<?php echo $setting_value; ?>" />
            <button type="submit" value="Change">Submit</button>
            </form>
        </main>
      </div>
    </div>
    <!-- <script type="text/javascript" src="./index.js"></script> -->

    </body>
</html>