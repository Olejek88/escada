<?php
 if ($_GET["ud"]=='1')
	{
	 $query = 'DELETE FROM device WHERE id='.$_GET["id"];
	 $a = mysql_query ($query,$i);
        }
 if ($_POST["conf"]=='1')
	{
	 $query = 'SELECT * FROM device';
	 if ($r = mysql_query ($query,$i))
	 while ($ur = mysql_fetch_row ($r))
		{ 
		 $ust=0;
		 $pthh='ust'.$ur[1];
		 if ($_POST[$pthh]=='on') $ust=1;
		 if ($ust!=$ur[21])
		    {
		     $query = 'UPDATE device SET devtim=devtim,ust='.$ust;
		     //echo $query.'<br>';
		     mysql_query ($query,$i);
		    }
        	} 
	}

 if ($_POST["frm"]=='1')
	{
	 if ($_POST["nid"]=='1')
		{ 
		 $query = 'SELECT startid FROM devicetype WHERE ids='.$_POST["n2"];
		 if ($a = mysql_query ($query,$i))
		 if ($uy = mysql_fetch_row ($a)) $startidd=$uy[0];

		 $query = 'SELECT MAX(idd) FROM device WHERE type='.$_POST["n2"];
		 if ($a = mysql_query ($query,$i))
		 if ($uy = mysql_fetch_row ($a)) $idd=$uy[0]+1;
		 if ($idd<100) $idd=$startidd+1;

                 if ($_POST["n5"]!='') $idd=$_POST["n5"];
		 if ($_POST["n9"]>0) $adr=$_POST["n9"]; else $adr=0x100;

		 if ($_POST["n1"]=='' || $_POST["n1"]=='1') $_POST["n1"]=1;
		 if ($_POST["n1"]>='1')
			{
			 for ($dd=1; $dd<=$_POST["n1"]; $dd++)
				{
			 	 if ($_POST["n2"]==1) { $name='BIT (flat='.$_POST["n10"].',str='.$_POST["n11"].')'; }
			 	 if ($_POST["n2"]==2) { $name='2IP (flat='.$_POST["n10"].')'; }
			 	 if ($_POST["n2"]==4) { $name='MEE (flat='.$_POST["n10"].')'; }
			 	 if ($_POST["n2"]==5) { $name='IRP (strut='.$_POST["n11"].')'; }
			 	 if ($_POST["n2"]==6) { $name='LK (ent='.$_POST["n11"].',str='.$_POST["n11"].')'; }
			 	 if ($_POST["n2"]==11) $name='Tekon';
				 if ($_POST["n4"]!='') $name=$_POST["n4"];

				 $query = 'INSERT INTO device SET name=\''.$name.'\',type=\''.$_POST["n2"].'\',interface=\''.$_POST["n3"].'\',
						  idd=\''.$idd.'\',protocol=\''.$_POST["n6"].'\',port=\''.$_POST["n7"].'\',
						  speed=\''.$_POST["n8"].'\',adr=\''.$adr.'\',flat=\''.$_POST["n10"].'\'';
				 //echo $query.'<br>';
				 $a = mysql_query ($query,$i);
                                 if ($_POST["n2"]==1)
					{
					 $query = 'INSERT INTO dev_bit SET device=\''.$idd.'\',rf_int_interval=\'3600\',ids_lk=\''.$_POST["n11"].'\',
						  ids_module=\''.$adr.'\',meas_interval=\'60\',integ_meas_cnt=\'60\',
						  flat_number=\''.$_POST["n10"].'\',strut_number=\''.$_POST["n12"].'\'';
					 //echo $query.'<br>';
					 $a = mysql_query ($query,$i);

					 $query = 'INSERT INTO channels SET name=\'Интегральное значение энтальпии\',device=\''.$idd.'\',opr=1,prm=1,pipe=0,shortname=\'Hинт,'.$adr.'\'';
					 $a = mysql_query ($query,$i);
					 $query = 'INSERT INTO channels SET name=\'Накопительное значение энтальпии\',device=\''.$idd.'\',opr=1,prm=1,pipe=1,shortname=\'Hнак,'.$adr.'\'';
					 $a = mysql_query ($query,$i);
					 $query = 'INSERT INTO channels SET name=\'Среднее значение температуры\',device=\''.$idd.'\',opr=1,prm=4,pipe=0,shortname=\'Тср,'.$adr.'\'';
					 $a = mysql_query ($query,$i);
					}
				 $idd++; $adr++;
				}
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

	<div id="divFutures" class="mpbox">
	<div>
	<div class="xscroll">
	<table class="datatable_simple js" data-largetable="500" data-extrafields="" data-pagesize="100" width="100%" border="0" cellpadding="1" cellspacing="1">
	<thead><tr class="datatable_header">
	<?php	
	 print '<th class="ds_name sort_none" align="center">ID</th>
		<th class="ds_last sort_none" align="center">Name</th>
	        <th class="ds_change sort_none" align="center">Interface</th>
	        <th class="ds_change sort_none" align="center">Protocol</th>
	        <th class="ds_change sort_none" align="center">Port</th>
	        <th class="ds_change sort_none" align="center">Speed</th>
	        <th class="ds_change sort_none" align="center">Addr</th>
	        <th class="ds_change sort_none" align="center">Type</th>
	        <th class="ds_change sort_none" align="center">IP/Num</th>
	        <th class="ds_change sort_none" align="center">Obj/Flat</th>
	        <th class="ds_change sort_none" align="center">Ust</th>
	        <th class="ds_change sort_none" align="center">Object</th>
	        <th class="ds_change sort_none" align="center"></th>';
	?>
	</tr></thead>
	<tbody>
	<form name="frm1" method="post" action="index.php?sel=devices">
	<?php
	 if ($_GET["type"]=='') $query = 'SELECT * FROM '.$device_table;
	 else $query = 'SELECT * FROM '.$device_table.' WHERE type='.$_GET["type"];
	 if ($a = mysql_query ($query,$i))
	 while ($uy = mysql_fetch_assoc ($a))
		{
		 print '<tr class="">
			<td class="ds_name qb_shad" align="left" nowrap="nowrap">'.$uy["device"].'</td>
			<td class="ds_name qb_shad" align="left" nowrap="nowrap"><a href="index.php?sel=channels&device='.$uy["device"].'">'.$uy["name"].'</a></td>';
		$query = 'SELECT * FROM interfaces WHERE id='.$uy["interface"];
	 	if ($a2 = mysql_query ($query,$i))
		if ($uy2 = mysql_fetch_row ($a2)) print '<td class="ds_name qb_shad" align="center" nowrap="nowrap">'.$uy2[1].'</td>';
		else print '<td class="ds_name qb_shad" align="center" nowrap="nowrap">unknown!</td>';

		$query = 'SELECT * FROM protocols WHERE protocol='.$uy["protocol"];
	 	if ($a2 = mysql_query ($query,$i))
		if ($uy2 = mysql_fetch_row ($a2)) print '<td class="ds_name qb_shad" align="left" nowrap="nowrap">'.$uy2[1].'</td>';

		if ($uy["interface"]==4) print '<td style="" class="ds_last qb_shad" align="center">eth</td>';
		else print '<td style="" class="ds_last qb_shad" align="center">ttyS'.$uy["port"].'</td>';
		print '<td style="" class="ds_last qb_shad" align="center">'.$uy["speed"].'</td>
		        <td style="" class="ds_last qb_shad" align="center">'.$uy["adr"].'</td>';
		$query = 'SELECT * FROM devicetype WHERE ids='.$uy["type"];
	 	if ($a2 = mysql_query ($query,$i))
		if ($uy2 = mysql_fetch_row ($a2)) print '<td class="ds_name qb_shad" align="left" nowrap="nowrap">'.$uy2[1].'</td>';

		print '<td style="" class="ds_last qb_shad" align="center">'.$uy["number"].'</td>
		        <td style="" class="ds_last qb_shad" align="center">'.$uy["flat"].'</td>';

		print '<td style="" class="ds_last qb_shad" align="center"><input type="checkbox" id="ust'.$uy["idd"].'" name="ust'.$uy["idd"].'" ';
	        if ($uy["ust"]) print 'checked';
		print '></td>';

		$query = 'SELECT * FROM objects WHERE id='.$uy["place"];
		if ($a2 = mysql_query ($query,$i))
		if ($uy2 = mysql_fetch_row ($a2)) $name=$uy2[1];
		if ($name=='')
			{
			 $query = 'SELECT * FROM objects WHERE id='.$uy["object"];
			 if ($a2 = mysql_query ($query,$i))
			 if ($uy2 = mysql_fetch_row ($a2)) $name=$uy2[1];
			}
		print '<td style="" class="ds_last qb_shad" align="center">'.$name.'</td>';
		//print '<td style="" class="ds_last qb_shad" align="center"><a href="index.php?sel=devices&ud=1&id='.$uy[0].'"><img border="0" src="files/delete.png"></td>';
		print '</tr>';
		$name='';
		}
	?>
	</tbody></table>
	<table><tr><td><input name="conf" type="submit" value="change" style="font-size:10px"><input name="conf" size=1 style="height:1;width:1;visibility:hidden" value="1"></td></tr></table>
	</form>

	<div class="loading22">
	<?php
	  print '<form name="frm1" method="post" action="index.php?sel=devices">';
	  print '<table border="0" cellpadding="1" cellspacing="1">';
	  print '<tr><td class="sidelistmenu" align=center><font style="color:white; font-weight:bold">Добавить устройство/каналы</font></td><td class="sidelistmenu" align=center><font style="color:white; font-weight:bold">Дерево каналов</font></td></tr>';
	  print '<tr><td style="width:940px" valign=top>
		 <table width=500px cellpadding=2 cellspacing=1 align=center class="datatable_simple js">';
	  print '<tr><td>Количество устройств</td><td><input name="n1" size=30 class=log style="height:14px"></td></tr>';
	  print '<tr><td>Тип устройств</td><td>
		 <select class="log" id="type" name="n2" style="height:14">';
	  $query = 'SELECT * FROM devicetype';
 	  if ($a = mysql_query ($query,$i))
	  while ($uy = mysql_fetch_row ($a))
		 print '<option value="'.$uy[4].'">'.$uy[1];
	  print '</select>
		 </td></tr>';
	  print '<tr><td>Интерфейс</td><td>
		 <select class=log id="type" name="n3" style="height:14">';
	  $query = 'SELECT * FROM interfaces';
 	  if ($a = mysql_query ($query,$i))
	  while ($uy = mysql_fetch_row ($a))
		 print '<option value="'.$uy[0].'">'.$uy[1];
	  print '</select>
		 </td></tr>';
	  print '<tr><td>Название (пусто автоматом)</td><td><input name="n4" size=30 class=log style="height:14px"></td></tr>';
	  print '<tr><td>Идентификатор (пусто автоматом)</td><td><input name="n5" size=5 class=log style="height:14px"></td></tr>';
	  print '<tr><td>Протокол</td><td>
		 <select class=log id="type" name="n6" style="height:14">';
	  $query = 'SELECT * FROM protocols';
 	  if ($a = mysql_query ($query,$i))
	  while ($uy = mysql_fetch_row ($a))
		 print '<option value="'.$uy[3].'">'.$uy[1];
	  print '</select></td></tr>';
	  print '<tr><td>Порт</td><td><input name="n7" size=5 class=log style="height:14px"></td></tr>';
	  print '<tr><td>Скорость</td><td>
		 <select class=log id="type" name="n8" style="height:14">';
		 print '<option value="20">20';
		 print '<option value="50">50';
		 print '<option value="300">300';
		 print '<option value="600">600';
		 print '<option value="1200">1200';
		 print '<option value="2400">2400';
		 print '<option value="4800">4800';
		 print '<option value="9600">9600';
		 print '<option value="19200">19200';
		 print '<option value="38400">38400';
		 print '<option value="57600">57600';
		 print '<option value="115200">115200';
	  print '</select>
		 </td></tr>';
	  print '<tr><td>Адрес (пусто автоматом)</td><td><input name="n9" size=5 class=log style="height:14px"></td></tr>';
	  print '<tr><td>Квартира (пусто автоматом)</td><td><input name="n10" size=5 class=log style="height:14px"></td></tr>';
	  print '<tr><td>ЛК ID</td><td>
		 <select class=log id="type" name="n11" style="height:14">';
	  $query = 'SELECT * FROM device WHERE type=6';
 	  if ($a = mysql_query ($query,$i))
	  while ($uy = mysql_fetch_row ($a))
		 print '<option value="'.$uy[7].'">'.$uy[20];
	  print '</select>
		 </td></tr>';
	  print '<tr><td>Стояк</td><td><input name="n12" size=5 class=log style="height:14px"></td></tr>';
	  print '<tr><td></td><td><input name="nid" size=1 style="height:1;width:1;visibility:hidden" value="1"></td></tr>';

	  print '<tr><td><input alt="add" name="Добавить" align=left type="submit"></td><td><input name="frm" size=1 style="height:1;width:1;visibility:hidden" value="1"></td></tr>';	    
	  print '</table></td>';
	  print '<td>';
	  include ("devices_tree.php");
	  print '</td></tr></table></form>';
	?>	
	</div></div>
	</div>
	</div>
	</td>
	</tr>
</tbody></table>