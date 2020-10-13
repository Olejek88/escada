<?php
 if ($_GET["ud"]=='1')
	{
	 $query = 'DELETE FROM register WHERE id='.$_GET["id"];
	 $a = mysql_query ($query,$i);
        }
?>

	<table class="columnLayout" style="width:940px">
	<tbody><tr>
	<td class="centercol2">
	<div class="bar firstBar">
	<?
	 print '<h2><a class="grey" href="index.php?sel=events">Events</a></h2>';
	?>
	<span class="link"><img src="files/dbl_arrow.gif" alt="*" width="19" height="12"></span>
	</div>

	<div id="divFutures" class="mpbox">
	<div>
	<div class="xscroll">
	<table class="datatable_simple js" data-largetable="500" data-extrafields="" data-pagesize="100" width="100%" border="0" cellpadding="1" cellspacing="1">
	<thead><tr class="datatable_header">
	<th class="ds_name sort_none" align="center">ID</th>
	        <th class="ds_change sort_none" align="center">Date</th>
		<th class="ds_last sort_none" align="center">Code</th>
	        <th class="ds_change sort_none" align="center">Device</th>
	        <th class="ds_change sort_none" align="center">Channel</th>
	        <th class="ds_change sort_none" align="center">Type</th>
	        <th class="ds_change sort_none" align="center">Value</th>
	        <th class="ds_change sort_none" align="center">Description</th>
	</tr></thead>
	<tbody>

	<?php
	 $query = 'SELECT * FROM register ORDER BY date DESC';
	 if ($a = mysql_query ($query,$i))
	 while ($uy = mysql_fetch_assoc ($a))
		{
		 $query = 'SELECT * FROM device WHERE idd='.$uy["device"];
		 if ($a2 = mysql_query ($query,$i))
		 if ($uy2 = mysql_fetch_assoc ($a2)) { $dev1=$uy2["name"]; $id1=$uy2["idd"]; }
		 $query = 'SELECT * FROM channels WHERE id='.$uy["channel"];
		 if ($a2 = mysql_query ($query,$i))
		 if ($uy2 = mysql_fetch_assoc ($a2)) { $name=$uy2["name"]; }

		 print '<tr class="">
			<td class="ds_name qb_shad" align="left" nowrap="nowrap">'.$uy["id"].'</td>
			<td class="ds_name qb_shad" align="center">'.$uy["date"].'</td>
			<td class="ds_name qb_shad" align="center">'.$uy["code"].'</td>
			<td class="ds_name qb_shad" align="left">'.$dev1.' ['.$uy["device"].']</td>';
		 if ($uy[7]==0) print '<td class="ds_name qb_shad" align="center">-</td>';
		 else  print '<td class="ds_name qb_shad" align="center">'.$name.'</td>';
          	 if ($uy["type"]=='1') print '<td style="padding-left:2px; padding-right:2px; background-color:#aaeeaa">информация</td>';
	  	 if ($uy["type"]=='2') print '<td style="padding-left:2px; padding-right:2px; background-color:#33dddd; align:center">предупреждение</td>';
      	         if ($uy["type"]=='3' || $uy["type"]=='0') print '<td style="padding-left:2px; padding-right:2px; background-color:#aa3333; align:center"><font style="color:white">ошибка</font></td>';
		 print '<td class="ds_name qb_shad" align="center">'.$uy["value"].'</td>
			<td class="ds_name qb_shad" align="center">'.$uy["descr"].'</td></tr>';
		}
	?>
	</tbody></table>
	</div></div>
	</div>
	</div>
	</td>
	</tr>
</tbody></table>