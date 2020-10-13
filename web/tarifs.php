<?php
 if ($_GET["ud"]=='1')
	{
	 $query = 'DELETE FROM tarifs WHERE id='.$_GET["id"];
	 $a = mysql_query ($query,$i);
        }
?>

	<table class="columnLayout" style="width:940px">
	<tbody><tr>
	<td class="centercol2">
	<div class="bar firstBar">
	<?
	 print '<h2><a class="grey" href="index.php?sel=tarifs">Tarifs</a></h2>';
	?>
	<span class="link"><img src="files/dbl_arrow.gif" alt="*" width="19" height="12"></span>
	</div>

	<div id="divFutures" class="mpbox">
	<div>
	<div class="xscroll">
	<table class="datatable_simple js" data-largetable="500" data-extrafields="" data-pagesize="100" width="100%" border="0" cellpadding="1" cellspacing="1">
	<thead><tr class="datatable_header">
	<th class="ds_name sort_none" align="center">ID</th>
	        <th class="ds_change sort_none" align="center">Year</th>
		<th class="ds_last sort_none" align="center">Object</th>
	        <th class="ds_change sort_none" align="center">Heat</th>
	        <th class="ds_change sort_none" align="center">Electro</th>
	        <th class="ds_change sort_none" align="center">GVS</th>
	        <th class="ds_change sort_none" align="center">HVS</th>
	        <th class="ds_change sort_none" align="center">Voda</th>
	</tr></thead>
	<tbody>

	<?php
	 $query = 'SELECT * FROM tarifs';
	 if ($a = mysql_query ($query,$i))
	 while ($uy = mysql_fetch_assoc ($a))
		{
		 $query = 'SELECT * FROM object WHERE id='.$uy["object"];
		 if ($a2 = mysql_query ($query,$i))
		 if ($uy2 = mysql_fetch_assoc ($a2)) { $obj=$uy2["name"]; $id1=$uy2["id"]; }

		 print '<tr class="">
			<td class="ds_name qb_shad" align="left" nowrap="nowrap">'.$uy["id"].'</td>
			<td class="ds_name qb_shad" align="center">'.$uy["year"].'</td>
			<td class="ds_name qb_shad" align="center">'.$obj.'</td>
			<td class="ds_name qb_shad" align="left">'.$uy["tepl"].']</td>
			<td class="ds_name qb_shad" align="left">'.$uy["elec"].']</td>
			<td class="ds_name qb_shad" align="left">'.$uy["gvs"].']</td>
			<td class="ds_name qb_shad" align="left">'.$uy["hvs"].']</td>
			<td class="ds_name qb_shad" align="left">'.$uy["voda"].']</td>';
		 print '</tr>';
		}
	?>
	</tbody></table>
	</div></div>
	</div>
	</div>
	</td>
	</tr>
</tbody></table>