<?php
/*
 * Easy PHP Tail 
 * by: Thomas Depole
 * v1.0
 * 
 * just fill in the varibles bellow, open in a web browser and tail away 
 */
$logFile = ""; // local path to log file
$interval = 1000; //how often it checks the log file for changes, min 100
$textColor = ""; //use CSS color

if(isset($_GET['displayLog'])) {
	$logFile = "/var/log/wsprrypi/" . $_GET['displayLog'];
}

// Don't have to change anything bellow
if(!$textColor) $textColor = "white";
if($interval < 100)  $interval = 100; 
if(isset($_GET['getLog'])) {
?>
<pre>
<?php echo file_get_contents($logFile); ?>
</pre>
<?php
} else {
?>
<html>

<head>
    <meta charset="utf-8">
    <title>Wsprry Pi</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="apple-touch-icon" sizes="180x180" href="apple-touch-icon.png">
    <link rel="icon" type="image/png" sizes="32x32" href="favicon-32x32.png">
    <link rel="icon" type="image/png" sizes="16x16" href="favicon-16x16.png">
    <link rel="manifest" href="site.webmanifest">
    <link rel="stylesheet" href="bootstrap.css">
    <link rel="stylesheet" href="custom.css">
    <script src="jquery-3.6.3.min.js"></script>
</head>

	<script>
		setInterval(readLogFile, <?php echo $interval; ?>);
		window.onload = readLogFile; 
		var pathname = window.location.pathname;
		var scrollLock = true;
		
		$(document).ready(function(){
			$('.disableScrollLock').click(function(){
				$("html,body").clearQueue()
				$(".disableScrollLock").hide();
				$(".enableScrollLock").show();
				scrollLock = false;
			});
			$('.enableScrollLock').click(function(){
				$("html,body").clearQueue()
				$(".enableScrollLock").hide();
				$(".disableScrollLock").show();
				scrollLock = true;
			});
		});
		function readLogFile(){
			$.get(pathname, { getLog : "true" }, function(data) {
				data = data.replace(new RegExp("\n", "g"), "<br />");
		        $("#log").html(data);
		        
		        if(scrollLock == true) { $('html,body').animate({scrollTop: $("#scrollLock").offset().top}, <?php echo $interval; ?>) };
		    });
		}
	</script>
	<body>
		<h4><?php echo $logFile; ?></h4>
		<div id="log"></div>
		<div id="scrollLock"> <input class="disableScrollLock" type="button" value="Disable Scroll Lock" /> <input class="enableScrollLock" style="display: none;" type="button" value="Enable Scroll Lock" /></div>
	
		<script src="bootstrap.bundle.min.js"></script>
    	<script src="fa.js" crossorigin="anonymous"></script>
	
	</body>
</html>
<?php  } ?>
