<?php
 include("local.php");
 $today=getdate();
 $i = mysql_connect ($mysql_host,$mysql_user,$mysql_password); $e=mysql_select_db ($mysql_db_name);
 $query = "set character_set_client='cp1251'"; mysql_query ($query,$i);
 $query = "set character_set_results='cp1251'"; mysql_query ($query,$i);
 $query = "set collation_connection='cp1251_general_ci'"; mysql_query ($query,$i);
 $error=0;
 if ($_GET["sel"]=='logout' )
    {
     setcookie("login", "", time() - 3600);
     setcookie("pass", "", time() - 3600);
     print '<script> window.location.href="index.php" </script>';

     // print '<meta http-equiv="Set-Cookie" content="login=\'\'">';
     // print '<meta http-equiv="Set-Cookie" content="pass=\'\'">';
     // print '<meta http-equiv="refresh" content="0,index.php">';     
    }
 if ($_POST["email"]!='' && $_POST["password"]!='' )
    {
      $query = 'SELECT * FROM users WHERE login=\''.$_POST["email"].'\' AND pass=\''.$_POST["password"].'\'';
      if ($a = mysql_query ($query,$i))
      if ($uy = mysql_fetch_row ($a))
	 {
	  $name=$uy[1];
	  $type=$uy[2];
          print '<meta http-equiv="Set-Cookie" content="login='.$_POST["email"].'">';
          print '<meta http-equiv="Set-Cookie" content="pass='.$_POST["password"].'">';
          print '<meta http-equiv="Set-Cookie" content="iduser='.$uy[0].'">';
          print '<meta http-equiv="Pragma" content="no-cashe">';
          print '<meta http-equiv="refresh" content="0,index.php">';
	 }
      else $error=1;
    }
?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html class=" js  placeholder" xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=Windows-1251">
<meta name="description" content="Webdata configuration interface">
<meta name="keywords" content="interface">
<title>Webdata configuration interface :: Main page</title>

<link rel="stylesheet" href="files/style.css" type="text/css">
<link rel="stylesheet" href="files/network.css" type="text/css">
<link rel="stylesheet" href="files/print.css" type="text/css" media="print">
<link rel="stylesheet" href="files/ui.css" type="text/css" media="all">
<script type="text/javascript" src="files/prototype.js"></script>
</head>
<body>
<div class="container">
    <div id="header_bar" class="loggedout">
    <span class="leftItems" style="padding-left: 10px;">Hello! <? print $_COOKIE["login"]; if ($_COOKIE["login"]!='') print ' [Admin]'; ?> </span>
    <span class="rightItems">
          <table style="float: right;">
          <tbody><tr><td>
          <span id="topbarLogin" class="header_bar_item hoverable">
	  <?php
              if ($_COOKIE["login"]=='') print '<span class="bcButton" style="cursor: default" onmousedown="" onclick="login.style.display=\'\';">Log In</span>';
	      else  print '<a href="index.php?sel=logout"><span class="bcButton" style="cursor: default" onmousedown="">LogOut</span></a>';
	  ?>
          <div class="options bastard_options" style="width:200px; display: none;" id="login">
          <form action="index.php" method="post" class="padder">
          <p>Your Username:<input name="email" type="text"></p>
          <p>Your Password:<input name="password" type="password"></p>
          <p><input value="Log In" type="submit"></p>
          </form>
          </div>
          </span>
          </td>
          </tr>
          </tbody></table>
    </span>
    </div>

    <center class="main">
    <div class="content" style="">
    <div class="menu">
	<ul>
	<?php
	 print '<li><a href="index.php" '; if ($_GET["sel"]=='') print 'class="current"'; print '>Main</a></li>';
	 print '<li><a href="index.php?sel=configuration" '; if ($_GET["sel"]=='channels' || $_GET["sel"]=='devices') print 'class="current"'; print '>Configuration</a></li>';
	 print '<li><a href="index.php?sel=data" '; if ($_GET["sel"]=='channel') print 'class="current"'; print '>Data</a></li>';
	 print '<li><a href="index.php?sel=info" '; if ($_GET["sel"]=='info') print 'class="current"'; print '>Info</a></li>';
	 print '<li><a href="index.php?sel=help" '; if ($_GET["sel"]=='help') print 'class="current"'; print '>Help</a></li>';
	?>
	<li><a href=""></a></li>
	<li><a href=""></a></li>
	<li><a href=""></a></li>
	<li><a href=""></a></li>
	</ul></div>

        <table class="menu_wrapper">
	<tbody><tr>
               <td class="sidemenu">
               <div class="sidemenu">
               <div class="sidemenuhead">Data</div>
               <div class="sidelistmenu">
               <ul>
                  <li><a href="index.php?sel=channels">Channels</a></li>
                  <li><a href="index.php?sel=devices">Devices</a></li>
                  <li><a href="index.php?sel=fields">Fields</a></li>
                  <li><a href="index.php?sel=flats">Flats</a></li>
                  <li><a href="index.php?sel=objects">Objects</a></li>
                  <li><a href="index.php?sel=events">Register</a></li>
                  <li><a href="index.php?sel=tarifs">Tarifs</a></li>
                  <li><a href="index.php?sel=users">Users</a></li>
                  <li><a href="index.php?sel=uzels">Uzels</a></li>
                  <li><a href="index.php?sel=logs">Logs</a></li>
		  <li><div class="sidemenuhead">System</div></li>
		  <li><a href="index.php?sel=protocol">Protocol</a></li>
		  <li><a href="index.php?sel=info&type=devicetype">Device Types</a></li>
		  <li><a href="index.php?sel=info&type=errors">Errors</a></li>
		  <li><a href="index.php?sel=info&type=protocols">Protocols</a></li>
		  <li><a href="index.php?sel=info&type=variables">Variables</a></li>
		</ul>
               <br>
               </div>
		<center><img src="files/webdata_m.jpg"></center>
               <div class="break"></div>
               </div>
               </td>

               <td class="maincol">
		<?php 
		   if ($_GET["sel"]=='') include ("main.php");
		   else { $file=$_GET["sel"].'.php'; include $file; }
		?>

		</td>
		</tr>
		</table>
</div>
<div style="clear:both"></div>
<div class="mp_spacer"></div>
</center>

<div id="footer">
<div id="copyright-info">
<p>Copyright &copy;2013, <a href="http://olejek.info">olejek. All Rights Reserved.</a>.</p>
</div>
<br><br><br>
</div>
</div>
</body></html>
