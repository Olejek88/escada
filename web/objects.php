<?php
 if ($_GET["ud"]=='1')
	{
	 $query = 'DELETE FROM objects WHERE id='.$_GET["id"];
	 //echo $query.'<br>';
	 $a = mysql_query ($query,$i);
        }
 if ($_POST["frm"]=='1')
	{
	 if ($_POST["nid"]=='3')
		{
		 $query = 'INSERT INTO objects SET name=\''.$_POST["n1"].'\', type=\''.$_POST["n2"].'\', build=\'.$_POST["n9"].\',strut=\''.$_POST["n4"].'\',block=\''.$_POST["n5"].'\',
						level=\''.$_POST["n6"].'\',flat=\''.$_POST["n7"].'\', nentr=\''.$_POST["n8"].'\',fname=\''.$n9.'\'';
		 $a = mysql_query ($query,$i);
		}
	}
?>

	<table class="columnLayout" style="width:940px">
	<tbody><tr>

	<td class="centercol2" colspan="2">

	<div class="bar firstBar">
	<?
	 print '<h2><a class="grey" href="index.php?sel=objects">Objects</a></h2>';
	?>
	<span class="link"><img src="files/dbl_arrow.gif" alt="*" width="19" height="12"></span>
	</div>

	<div id="divFutures" class="mpbox">
	<div>
	<div class="xscroll">

	<table class="datatable_simple js" data-largetable="500" data-extrafields="" data-pagesize="100" width="100%" border="0" cellpadding="1" cellspacing="1">
	<thead><tr class="datatable_header">
	<?php	
	 print '<th class="ds_name sort_none" align="center">Name</th>
		<th class="ds_last sort_none" align="center">Type</th>
	        <th class="ds_change sort_none" align="center">Building</th>
	        <th class="ds_change sort_none" align="center">Strut</th>
	        <th class="ds_change sort_none" align="center">Block</th>
	        <th class="ds_change sort_none" align="center">Level</th>
	        <th class="ds_change sort_none" align="center">Flat</th>
	        <th class="ds_change sort_none" align="center">Ident</th>
	        <th class="ds_change sort_none" align="center">Object</th>
	        <th class="ds_change sort_none" align="center">Entrance</th>
	        <th class="ds_change sort_none" align="center">x</th>
	        <th class="ds_change sort_none" align="center">y</th>
	        <th class="ds_change sort_none" align="center"></th>';
	?>
	</tr></thead>
	<tbody>

	<?php
	 $query = 'SELECT * FROM objects';
	 if ($a = mysql_query ($query,$i))
	 while ($uy = mysql_fetch_assoc ($a))
		{
		 print '<tr class="">
			<td class="ds_name qb_shad" align="left" nowrap="nowrap">'.$uy["name"].'</td>';
		 if ($uy["x"]>0 && $uy["y"]>0)
			{
			 if ($uy["type"]=='0') print '<td style="" class="ds_last qb_shad" align="left">Неизвестно</td>';
			 if ($uy["type"]=='1') print '<td style="" class="ds_last qb_shad" align="left"></td>';
			 if ($uy["type"]=='2') print '<td style="" class="ds_last qb_shad" align="left">Больницы</td>';
			 if ($uy["type"]=='3') print '<td style="" class="ds_last qb_shad" align="left">МДОУ</td>';
			 if ($uy["type"]=='4') print '<td style="" class="ds_last qb_shad" align="left">МУДОД</td>';
			 if ($uy["type"]=='5') print '<td style="" class="ds_last qb_shad" align="left">Школы</td>';
			 if ($uy["type"]=='6') print '<td style="" class="ds_last qb_shad" align="left">Административные</td>';
			}
		 else	{
			 if ($uy["type"]=='0') print '<td style="" class="ds_last qb_shad" align="left">Неизвестно</td>';
			 if ($uy["type"]=='1') print '<td style="" class="ds_last qb_shad" align="left">Объект/Здание/Завод</td>';
			 if ($uy["type"]=='2') print '<td style="" class="ds_last qb_shad" align="left">Подъезд/Вход/Цех</td>';
			 if ($uy["type"]=='3') print '<td style="" class="ds_last qb_shad" align="left">Этаж/Участок</td>';
			 if ($uy["type"]=='4') print '<td style="" class="ds_last qb_shad" align="left">Стояк/Трубопровод</td>';
			 if ($uy["type"]=='5') print '<td style="" class="ds_last qb_shad" align="left">Блок/Шкаф</td>';
			 if ($uy["type"]=='6') print '<td style="" class="ds_last qb_shad" align="left">Квартира</td>';
			}
		 print '<td style="" class="ds_last qb_shad" align="center">'.$uy["build"].'</td>
		        <td style="" class="ds_last qb_shad" align="center">'.$uy["strut"].'</td>
		        <td style="" class="ds_last qb_shad" align="center">'.$uy["block"].'</td>
		        <td style="" class="ds_last qb_shad" align="center">'.$uy["level"].'</td>
		        <td style="" class="ds_last qb_shad" align="center">'.$uy["flat"].'</td>
		        <td style="" class="ds_last qb_shad" align="center">'.$uy["id"].'</td>
		        <td style="" class="ds_last qb_shad" align="center">'.$uy["idd"].'</td>
		        <td style="" class="ds_last qb_shad" align="center">'.$uy["nentr"].'</td>
		        <td style="" class="ds_last qb_shad" align="center">'.$uy["x"].'</td>
		        <td style="" class="ds_last qb_shad" align="center">'.$uy["y"].'</td>
		        <td style="" class="ds_last qb_shad" align="center"><a href="index.php?sel=objects&ud=1&id='.$uy["id"].'"><img border="0" src="files/delete.png"></td>
			</tr>';
		}
	?>
	</tbody></table>

	<div class="loading22">
	<?php
	  print '<form name="frm1" method="post" action="index.php?sel=objects">';
	  print '<table border="0" cellpadding="1" cellspacing="1">';
	  print '<tr><td class="sidelistmenu" align=center><font style="color:white; font-weight:bold">Добавить объект</font></td><td class="sidelistmenu" align=center><font style="color:white; font-weight:bold">Дерево объектов</font></td></tr>';
	  print '<tr><td style="width:600px" valign=top>
		 <table width=600px cellpadding=2 cellspacing=1 align=center>';

	  print '<tr><td>Название объекта</td><td><input name="n1" size=30 class="log" style="height:18px"></td></tr>';
	  print '<tr><td>Тип</td><td>
		 <select class="log" id="type" name="n2" style="height:18">
		 <option value="0">Неизвестно
		 <option value="1">Объект/Здание/Завод
		 <option value="2">Подъезд/Вход/Цех
		 <option value="3">Этаж/Участок
		 <option value="4">Стояк/Трубопровод
		 <option value="5">Блок/Шкаф
		 <option value="6">Квартира
		 <option value="2">Больницы
		 <option value="3">МДОУ
		 <option value="4">МУДОД
		 <option value="5">Школы
		 <option value="6">Административные
		 </select>
		 </td></tr>';
	  print '<tr><td>Идентификатор объекта</td><td><input name="n3" size=5 class=log style="height:18px"></td></tr>';
	  print '<tr><td>Стояк / Трубопровод / Ветка</td><td>
		 <select class=log id="type" name="n4" style="height:18">';
	  $query = 'SELECT * FROM objects WHERE type=4';
 	  if ($a = mysql_query ($query,$i))
	  while ($uy = mysql_fetch_row ($a))
	      	{
		 print '<option value="'.$uy[4].'">'.$uy[11];
		}
	  print '</select>';
	  print '</td></tr>';
	  print '<tr><td>Объект/Здание/Завод</td><td>
		 <select class=log id="type" name="n9" style="height:18">
		 <option value="0">Не относится';
	  $query = 'SELECT * FROM objects WHERE type=1';
	  if ($a = mysql_query ($query,$i))
	  while ($uy = mysql_fetch_row ($a))
	      	{
		 print '<option value="'.$uy[0].'">'.$uy[1];
		}
	  print '</select>';
	  print '</td></tr>';

	  print '<tr><td>Подъезд/Вход/Цех</td><td>
		 <select class=log id="type" name="n8" style="height:18">
		 <option value="0">Не относится';
	  $query = 'SELECT * FROM objects WHERE type=2';
	  if ($a = mysql_query ($query,$i))
	  while ($uy = mysql_fetch_row ($a))
	      	{
		 print '<option value="'.$uy[4].'">'.$uy[1];
		}
	  print '</select>';
	  print '</td></tr>';
	  print '<tr><td>Этаж/Участок</td><td>
		 <select class=log id="type" name="n6" style="height:18">
		 <option value="0">Не относится';
	  $query = 'SELECT * FROM objects WHERE type=3';
 	  if ($a = mysql_query ($query,$i))
	  while ($uy = mysql_fetch_row ($a))
	      	{
		 print '<option value="'.$uy[4].'">'.$uy[11];
		}
	  print '</select>';
	  print '</td></tr>';
	  print '<tr><td>Блок/Шкаф</td><td>
		 <select class=log id="type" name="n5" style="height:18">
		 <option value="0">Нет блока
		 <option value="1">Левый блок
		 <option value="2">Правый блок
	  	 </select>';
	  print '</td></tr>';
	  print '<tr><td>Квартира</td><td>
		 <select class=log id="type" name="n7" style="height:18">
		 <option value="0">Не относится';
	  $query = 'SELECT * FROM objects WHERE type=6';
 	  if ($a = mysql_query ($query,$i))
	  while ($uy = mysql_fetch_row ($a))
	      	{
		 print '<option value="'.$uy[4].'">'.$uy[11];
		}
	  print '</select>';
	  print '</td></tr>';

	  print '<tr><td></td><td><input name="nid" size=1 style="height:1;width:1;visibility:hidden" value="3"></td></tr>';
	  print '<tr><td><input alt="add" name="Добавить" align=left type="submit"></td><td><input name="frm" size=1 style="height:1;width:1;visibility:hidden" value="1"></td></tr>';	    
	  print '</table></td>';
	  print '<td>';
	  include ("objects_tree.php");
	  print '</td></tr></table></form>';
	?>	
	</div>
	</div></div></div>
	</td>
	</tr>
</tbody></table>