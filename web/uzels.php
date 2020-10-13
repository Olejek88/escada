<?php
 if ($_GET["ud"]=='1')
	{
	 $query = 'DELETE FROM users WHERE id='.$_GET["id"];
	 $a = mysql_query ($query,$i);
        }
?>

	<table class="columnLayout" style="width:940px">
	<tbody><tr>
	<td class="centercol2">
	<div class="bar firstBar">
	<?
	 print '<h2><a class="grey" href="index.php?sel=users">Users</a></h2>';
	?>
	<span class="link"><img src="files/dbl_arrow.gif" alt="*" width="19" height="12"></span>
	</div>

	<div id="divFutures" class="mpbox">
	<div>
	<div class="xscroll">
	<table class="datatable_simple js" data-largetable="500" data-extrafields="" data-pagesize="100" width="100%" border="0" cellpadding="1" cellspacing="1">
	<thead><tr class="datatable_header">
	<th class="ds_name sort_none" align="center">ID</th>
	        <th class="ds_change sort_none" align="center">Name</th>
		<th class="ds_last sort_none" align="center">Type</th>
	        <th class="ds_change sort_none" align="center">Login</th>
	        <th class="ds_change sort_none" align="center">Password</th>
	        <th class="ds_change sort_none" align="center">Date</th>
	        <th class="ds_change sort_none" align="center">IP</th>
	</tr></thead>
	<tbody>

	<?php
	 $query = 'SELECT * FROM users';
	 if ($a = mysql_query ($query,$i))
	 while ($uy = mysql_fetch_assoc ($a))
		{
		 print '<tr class="">
			<td class="ds_name qb_shad" align="left" nowrap="nowrap">'.$uy["id"].'</td>
			<td class="ds_name qb_shad" align="center">'.$uy["name"].'</td>';
		if ($uy["type"]==0) print '<td class="ds_name qb_shad" align="center" style="background-color:#ffffff">никто</td>';
		if ($uy["type"]==1) print '<td class="ds_name qb_shad" align="center" style="background-color:#aaeeaa">пользователь</td>';
		if ($uy["type"]==2) print '<td class="ds_name qb_shad" align="center" style="background-color:#4488ff">инженер</td>';
		if ($uy["type"]==3) print '<td class="ds_name qb_shad" align="center" style="background-color:#33dddd">администратор</td>';
		print  '<td class="ds_name qb_shad" align="center">'.$uy["login"].'</td>
			<td class="ds_name qb_shad" align="center">'.$uy["pass"].'</td>
			<td class="ds_name qb_shad" align="center">'.$uy["date"].'</td>
			<td class="ds_name qb_shad" align="center">'.$uy["ip"].'</td>';
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