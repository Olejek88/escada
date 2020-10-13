<?php
 if ($_GET["ud"]=='1')
	{
	 $query = 'DELETE FROM field WHERE id='.$_GET["id"];
	 $a = mysql_query ($query,$i);
        }
 if ($_POST["frm"]=='1')
	{
	 $query = 'INSERT INTO field SET mnem=\''.$_POST["n1"].'\', x=\''.$_POST["n2"].'\', y=\''.$_POST["n3"].'\',
			  id1=\''.$_POST["n4"].'\', id2=\''.$_POST["n5"].'\',  flat=\''.$_POST["n6"].'\',
			  type=\''.$_POST["n7"].'\', pip=\''.$_POST["n8"].'\', strut=\''.$_POST["n9"].'\'';
	 echo $query.'<br>';
	 $a = mysql_query ($query,$i);
	}
?>

	<table class="columnLayout" style="width:940px">
	<tbody><tr>
	<td class="centercol2">
	<div class="bar firstBar">
	<?
	 print '<h2><a class="grey" href="index.php?sel=fields">Calculations</a></h2>';
	?>
	<span class="link"><img src="files/dbl_arrow.gif" alt="*" width="19" height="12"></span>
	</div>

	<div id="divFutures" class="mpbox">
	<div>
	<div class="xscroll">
	<table class="datatable_simple js" data-largetable="500" data-extrafields="" data-pagesize="100" width="100%" border="0" cellpadding="1" cellspacing="1">
	<thead><tr class="datatable_header">
	<th class="ds_name sort_none" align="center">ID</th>
		<th class="ds_last sort_none" align="center">Mnem</th>
		<th class="ds_last sort_none" align="center">x,y</th>
	        <th class="ds_change sort_none" align="center">device 1[id1]</th>
	        <th class="ds_change sort_none" align="center">device 2[id2]</th>
	        <th class="ds_change sort_none" align="center">flat</th>
	        <th class="ds_change sort_none" align="center">type</th>
	        <th class="ds_change sort_none" align="center">pipe</th>
	        <th class="ds_change sort_none" align="center">strut</th>
	</tr></thead>
	<tbody>

	<?php
	 $query = 'SELECT * FROM field';
	 if ($a = mysql_query ($query,$i))
	 while ($uy = mysql_fetch_row ($a))
		{
		 $query = 'SELECT * FROM device WHERE idd='.$uy[4];
		 if ($a2 = mysql_query ($query,$i))
		 if ($uy2 = mysql_fetch_row ($a2)) { $dev1=$uy2[20]; $id1=$uy2[1]; }
		 $query = 'SELECT * FROM device WHERE idd='.$uy[5];
		 if ($a2 = mysql_query ($query,$i))
		 if ($uy2 = mysql_fetch_row ($a2)) { $dev2=$uy2[20]; $id2=$uy2[1]; }

		 print '<tr class="">
			<td class="ds_name qb_shad" align="left" nowrap="nowrap">'.$uy[0].'</td>
			<td class="ds_name qb_shad" align="center">'.$uy[1].'</td>
			<td class="ds_name qb_shad" align="center">'.$uy[2].','.$uy[3].'</td>
			<td class="ds_name qb_shad" align="center" nowrap="nowrap">'.$dev1.' ['.$uy[4].']</td>
			<td class="ds_name qb_shad" align="left" nowrap="nowrap">'.$dev2.' ['.$uy[5].']</td>
			<td class="ds_name qb_shad" align="left" nowrap="nowrap">'.$uy[6].'</td>';
		 if ($uy[7]==1) print '<td class="ds_name qb_shad" align="left" nowrap="nowrap">разница энтальпий</td>';
		 else if ($uy[7]==2) print '<td class="ds_name qb_shad" align="left" nowrap="nowrap">показания</td>';
		 else if ($uy[7]==3) print '<td class="ds_name qb_shad" align="left" nowrap="nowrap">номер</td>';
		 print '<td class="ds_name qb_shad" align="center" nowrap="nowrap">'.$uy[8].'</td>
			<td class="ds_name qb_shad" align="center" nowrap="nowrap">'.$uy[9].'</td>';
		 print '<td style="" class="ds_last qb_shad" align="center"><a href="index.php?sel=fields&ud=1&id='.$uy[0].'"><img border="0" src="files/delete.png"></td>
			</tr>';
		}
	?>
	</tbody></table>
	<table><tr><td><a style="cursor:pointer" onclick="load22.style.visibility='visible'">add fields</a></td></tr></table>
	<div class="loading22" id="load22" name="load22" style="visibility:hidden">
	<?php
	  print '<form name="frm1" method="post" action="index.php?sel=fields">';
	  print '<table border="0" cellpadding="1" cellspacing="1">';
	  print '<tr><td class="sidelistmenu" align=center><font style="color:white; font-weight:bold">Добавить поля вывода/вычислений</font></td></tr>';
	  print '<tr><td style="width:940px" valign=top>
		 <table width=940px cellpadding=2 cellspacing=1 align=center class="datatable_simple js">';
	  print '<tr><td>Мнемосхема/Экран</td><td><input name="n1" size=5 class=log style="height:14px"></td></tr>';
	  print '<tr><td>Координата X</td><td><input name="n2" size=5 class=log style="height:14px"></td></tr>';
	  print '<tr><td>Координата Y</td><td><input name="n3" size=5 class=log style="height:14px"></td></tr>';
	  print '<tr><td>Устройство 1</td><td>
		 <select class="log" id="type" name="n4" style="height:14">';
	  $query = 'SELECT * FROM device WHERE type=1 ORDER BY adr';
 	  if ($a = mysql_query ($query,$i))
	  while ($uy = mysql_fetch_row ($a))
		 print '<option value="'.$uy[1].'">'.$uy[20].' ['.$uy[1].']';
	  print '</select>
		 </td></tr>';
	  print '<tr><td>Устройство 2</td><td>
		 <select class="log" id="type" name="n5" style="height:14">';
	  $query = 'SELECT * FROM device WHERE type=1 ORDER BY adr';
 	  if ($a = mysql_query ($query,$i))
	  while ($uy = mysql_fetch_row ($a))
		 print '<option value="'.$uy[1].'">'.$uy[20].' ['.$uy[1].']';
	  print '</select>
		 </td></tr>';
	  print '<tr><td>Квартира</td><td><input name="n6" size=5 class=log style="height:14px"></td></tr>';
	  print '<tr><td>Тип</td><td>
		 <select class="log" id="type" name="n7" style="height:14">
		 <option value="1">разница энтальпий
		 <option value="2">показания
		 <option value="3">номер
		 </select>';
	  print '</td></tr>';
	  print '<tr><td>Труба</td><td><input name="n8" size=5 class=log style="height:14px"></td></tr>';
	  print '<tr><td>Стояк</td><td><input name="n9" size=5 class=log style="height:14px"></td></tr>';
	  print '<tr><td><input alt="add" name="Добавить" align=left type="submit"></td><td><input name="frm" size=1 style="height:1;width:1;visibility:hidden" value="1"></td></tr>';	    
	  print '</table></form>';
	?>	
	</div></div>
	</div>
	</div>
	</td>
	</tr>
</tbody></table>