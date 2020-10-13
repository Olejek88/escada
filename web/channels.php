<?php
 if ($_GET["ud"]=='1')
	{
	 $query = 'DELETE FROM channels WHERE id='.$_GET["id"];
	 $a = mysql_query ($query,$i);
        }
 if ($_POST["frm"]=='1')
	{
	 if ($_POST["n1"]=='') $_POST["n1"]=$_POST["n1_1"];

	 $query = 'SELECT * FROM var2 WHERE id='.$_POST["n8"];
 	 if ($a = mysql_query ($query,$i))
	 if ($uy = mysql_fetch_row ($a)) { $prm=$uy[2]; $pipe=$uy[3]; }

	 $query = 'INSERT INTO channels SET name=\''.$_POST["n1"].'\', device=\''.$_POST["n2"].'\', opr=\'1\',
			  edizm=\''.$_POST["n3"].'\', adr=\''.$_POST["n4"].'\',  addr1=\''.$_POST["n5"].'\',
			  addr2=\''.$_POST["n6"].'\', addr3=\''.$_POST["n7"].'\',
			  prm=\''.$prm.'\',pipe=\''.$pipe.'\', shortname=\''.$_POST["n9"].'\'';
	 //echo $query.'<br>';
	 $a = mysql_query ($query,$i);
	}

 if ($_POST["conf"]=='2')
	{
	 $query = 'SELECT * FROM channels';
 	 if ($a = mysql_query ($query,$i))
	 while ($uy = mysql_fetch_row ($a)) 
	    { 
    	     $opr_=0;
	     $opr='opr'.$uy[0];
	     if ($_POST[$opr]=='on' && $uy[3]==0)
		{
	         $query = 'UPDATE channels SET opr=1 WHERE id='.$uy[0];
		 //echo $query.'<br>';
	    	 $a = mysql_query ($query,$i);
		}
	     if ($_POST[$opr]!='on' && $uy[3]==1)
		{
	         $query = 'UPDATE channels SET opr=0 WHERE id='.$uy[0];
		 //echo $query.'<br>';
	    	 $a = mysql_query ($query,$i);
		}
	    }
	}
?>
	<table class="columnLayout" style="width:940px">
	<tbody><tr>
	<td class="centercol2">
	<div class="bar firstBar">
	<?
	 print '<h2><a class="grey" href="index.php?sel=devices">Devices</a></h2>';
	?>
	<span class="link"><img src="files/dbl_arrow.gif" alt="*" width="19" height="12"></span>
	</div>

	<div id="divLeaders" class="mpbox">
	<div class="xscroll">
	<form name="frm1" method="post" action="index.php?sel=channels">
	<table class="datatable js" width="300px" border="0" cellpadding="1" cellspacing="1">
	<tbody>
	<?php
	 $query = 'SELECT * FROM device WHERE idd='.$_GET["device"];
	 if ($a2 = mysql_query ($query,$i))
	 if ($uy2 = mysql_fetch_row ($a2))
	    {
	     print '<tr class=""><td class="ds_symbol qb_shad" align="left" nowrap="nowrap">&nbsp;device</td>
		    <td class="ds_pctchange qb_line" align="left" nowrap="nowrap" valign="center"> ';
	     $query = 'SELECT * FROM devicetype WHERE ids='.$uy2[8];
 	     if ($a = mysql_query ($query,$i))
	     if ($uy = mysql_fetch_row ($a))	$type=$uy[1];
	     print $type;
	     print '</td><td class="ds_pctchange qb_line" align="left" nowrap="nowrap" valign="center">'.$uy2[20].' ['.$uy2[1].']</td></tr>';
	     print '<tr class=""><td class="ds_symbol qb_shad" align="left" nowrap="nowrap">&nbsp;address</td>
		    <td class="ds_pctchange qb_line" align="left" nowrap="nowrap" valign="center">'.$uy2[7].'</td>
		    <td class="ds_pctchange qb_line" align="left" nowrap="nowrap" valign="center">'.$uy2[9].'</td></tr>';
	     print '<tr class=""><td class="ds_symbol qb_shad" align="left" nowrap="nowrap">&nbsp;interface/protocol</td>
		    <td class="ds_pctchange qb_line" align="left" nowrap="nowrap" valign="center">';
	     $query = 'SELECT * FROM interfaces WHERE id='.$uy2[3];
 	     if ($a = mysql_query ($query,$i))
	     if ($uy = mysql_fetch_row ($a))	$iface=$uy[1];
	     print $iface;
	     print '</td><td class="ds_pctchange qb_line" align="left" nowrap="nowrap" valign="center">'.$uy2[4].'</td></tr>';

	     if ($uy2[5]==255) $uy2[5]='eth';
	     print '<tr class=""><td class="ds_symbol qb_shad" align="left" nowrap="nowrap">&nbsp;port/speed</td>
		    <td class="ds_pctchange qb_line" align="left" nowrap="nowrap" valign="center">'.$uy2[5].'</td>
		    <td class="ds_pctchange qb_line" align="left" nowrap="nowrap" valign="center">'.$uy2[6].'</td></tr>';
	     print '<tr class=""><td class="ds_symbol qb_shad" align="left" nowrap="nowrap">&nbsp;last request/ device time</td>
		    <td class="ds_pctchange qb_line" align="left" nowrap="nowrap" valign="center">'.$uy2[12].'</td>
		    <td class="ds_pctchange qb_line" align="left" nowrap="nowrap" valign="center">'.$uy2[16].'</td></tr>';

	     $query = 'SELECT * FROM objects WHERE id='.$uy2[24];
 	     if ($a = mysql_query ($query,$i))
	     if ($uy = mysql_fetch_row ($a))	$place=$uy[1];

	     print '<tr class=""><td class="ds_symbol qb_shad" align="left" nowrap="nowrap">&nbsp;place</td>
		    <td class="ds_pctchange qb_line" align="left" nowrap="nowrap" valign="center">'.$place.'</td>
		    <td class="ds_pctchange qb_line" align="left" nowrap="nowrap" valign="center"></td></tr>';
	    }
        ?>
	</tbody></table>
	<table><tr><td><input name="conf" type="submit" value="change" style="font-size:10px"><input name="conf" size=1 style="height:1;width:1;visibility:hidden" value="2"></td></tr></table>
	</form>

	<div id="divFutures" class="mpbox">
	<div>
	<div class="xscroll">
	<table class="datatable_simple js" data-largetable="500" data-extrafields="" data-pagesize="100" width="100%" border="0" cellpadding="1" cellspacing="1">
	<thead><tr class="datatable_header">
	<?php
	 print '<th class="ds_name sort_none" align="center">ID</th>
		<th class="ds_last sort_none" align="center">Name</th>
		<th class="ds_last sort_none" align="center">Device[DevID]</th>
	        <th class="ds_change sort_none" align="center">Opr</th>
	        <th class="ds_change sort_none" align="center">AInt</th>
	        <th class="ds_change sort_none" align="center">ACur</th>
	        <th class="ds_change sort_none" align="center">AHours</th>
	        <th class="ds_change sort_none" align="center">ADays</th>
	        <th class="ds_change sort_none" align="center">Prm</th>
	        <th class="ds_change sort_none" align="center">Pipe</th>
	        <th class="ds_change sort_none" align="center">Last Currents</th>
	        <th class="ds_change sort_none" align="center">Last Hours</th>
	        <th class="ds_change sort_none" align="center">SName</th>';
	?>
	</tr></thead>
	<tbody>

	<?php
	 print '<form name="frm1" method="post" action="index.php?sel=channels">';

	 if ($_GET["device"]=='') $query = 'SELECT * FROM channels';
	 else $query = 'SELECT * FROM channels WHERE device='.$_GET["device"];

	 if ($_GET["prm"]!='') $query = 'SELECT * FROM channels WHERE prm='.$_GET["prm"];

	 if ($a = mysql_query ($query,$i))
	 while ($uy = mysql_fetch_assoc ($a))
		{
		 $query = 'SELECT * FROM '.$device_table.' WHERE device='.$uy["device"];
		 if ($a2 = mysql_query ($query,$i))
		 if ($uy2 = mysql_fetch_assoc ($a2)) $devname=$uy2["name"];
		 $query = 'SELECT * FROM var2 WHERE prm='.$uy["prm"].' AND pipe='.$uy["pipe"];
	 	 if ($a2 = mysql_query ($query,$i))
 		 if ($uy2 = mysql_fetch_assoc ($a2)) $var=$uy2["name"];

		 print '<tr class="">
			<td class="ds_name qb_shad" align="left" nowrap="nowrap">'.$uy["id"].'</td>
			<td class="ds_name qb_shad" align="left"><a href="index.php?sel=channel&id='.$uy["id"].'">'.$uy["name"].'</a></td>
			<td class="ds_name qb_shad" align="left" nowrap="nowrap">'.$devname.'['.$uy["device"].']</td>';
		 print '<td class="ds_name qb_shad" align="center" nowrap="nowrap">
			<input type="checkbox" id="opr'.$uy["id"].'" name="opr'.$uy["id"].'" ';
			if ($uy["opr"]) print 'checked';
			print '></td>';
                 print '<td class="ds_name qb_shad" align="center" nowrap="nowrap">'.$uy["adr"].'</td>
			<td class="ds_name qb_shad" align="center" nowrap="nowrap">'.$uy["addr1"].'</td>
			<td class="ds_name qb_shad" align="center" nowrap="nowrap">'.$uy["addr2"].'</td>';
		 print '<td class="ds_name qb_shad" align="center" nowrap="nowrap">'.$uy["addr3"].'</td>';
		 print '<td class="ds_name qb_shad" align="center" nowrap="nowrap">'.$uy["prm"].'</td>';
		 print '<td class="ds_name qb_shad" align="center" nowrap="nowrap">'.$uy["pipe"].'</td>';
		 print '<td class="ds_name qb_shad" align="center" >'.$uy["lastcurrents"].'</td>';
		 print '<td class="ds_name qb_shad" align="center" nowrap="nowrap">'.$uy["lastdays"].'</td>';
		 //print '<td class="ds_name qb_shad" align="center" >'.$uy[12].'</td>';
		 //print '<td class="ds_name qb_shad" align="center" >'.$uy[13].'</td>';
		 //print '<td class="ds_name qb_shad" align="center" >'.$uy[14].'</td>';
		 print '<td class="ds_name qb_shad" align="center" nowrap="nowrap">'.$uy["shortname"].'</td>
		        <td style="" class="ds_last qb_shad" align="center"><a href="index.php?sel=channels&ud=1&id='.$uy[0].'"><img border="0" src="files/delete.png"></td>
			</tr>';
		}
	print '<tr><td><input name="conf" type="submit" value="change" style="font-size:10px"><input name="conf" size=1 style="height:1;width:1;visibility:hidden" value="2"></td></tr>';
	print '</form>';
	?>
	</tbody></table>
	<table><tr><td><a style="cursor:pointer" onclick="load22.style.visibility='visible'"><img src="files/add.png" border=0></a></td></tr></table>
	<div class="loading22" id="load22" name="load22" style="visibility:hidden">
	<?php
	  print '<form name="frm1" method="post" action="index.php?sel=channels">';
	  print '<table border="0" cellpadding="1" cellspacing="1">';
	  print '<tr><td class="sidelistmenu" align=center><font style="color:white; font-weight:bold">Добавить устройство/каналы</font></td></tr>';
	  print '<tr><td style="width:940px" valign=top>
		 <table width=940px cellpadding=2 cellspacing=1 align=center class="datatable_simple js">';
	  print '<tr><td>Название канала</td><td><input name="n1" size=50 class=log style="height:14px"></td></tr>';
	  print '<tr><td>Или из списка</td><td>
		 <select class="log" id="type" name="n1_1" style="height:14">';
	  $query = 'SELECT * FROM channels GROUP BY name';
 	  if ($a = mysql_query ($query,$i))
	  while ($uy = mysql_fetch_row ($a))
		 print '<option value="'.$uy[1].'">'.$uy[1];
	  print '</select>
		 </td></tr>';
	  print '<tr><td>Устройство</td><td>
		 <select class="log" id="type" name="n2" style="height:14">';
	  $query = 'SELECT * FROM device';
 	  if ($a = mysql_query ($query,$i))
	  while ($uy = mysql_fetch_row ($a))
		 print '<option value="'.$uy[1].'">'.$uy[20].' ['.$uy[1].']';
	  print '</select>
		 </td></tr>';
	  print '<tr><td>Адрес/смещение накопительных архивов</td><td><input name="n3" size=10 class=log style="height:14px"></td></tr>';
	  print '<tr><td>Адрес/смещение текущих значений</td><td><input name="n4" size=10 class=log style="height:14px"></td></tr>';
	  print '<tr><td>Адрес/смещение часовых значений</td><td><input name="n5" size=10 class=log style="height:14px"></td></tr>';
	  print '<tr><td>Адрес/смещение дневных значений</td><td><input name="n6" size=10 class=log style="height:14px"></td></tr>';
	  print '<tr><td>Адрес/смещение значений по месяцам</td><td><input name="n7" size=10 class=log style="height:14px"></td></tr>';
	  print '<tr><td>Параметр</td><td>
		 <select class=log id="type" name="n8" style="height:14">';
	  $query = 'SELECT * FROM var2';
 	  if ($a = mysql_query ($query,$i))
	  while ($uy = mysql_fetch_row ($a))
		 print '<option value="'.$uy[0].'">'.$uy[1];
	  print '</select>
		 </td></tr>';
	  print '<tr><td>Короткое имя канала</td><td><input name="n9" size=30 class=log style="height:14px"></td></tr>';
	  print '<tr><td><input alt="add" name="Добавить" align=left type="submit"></td><td><input name="frm" size=1 style="height:1;width:1;visibility:hidden" value="1"></td></tr>';	    
	  print '</table></form>';
	?>	
	</div></div>
	</div>
	</div>
	</td>
	</tr>
</tbody></table>