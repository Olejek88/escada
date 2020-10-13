	<table class="columnLayout">
	<tbody><tr>
	<td class="centercol2">
	<div class="bar firstBar">
	<?
	 if ($_GET["type"]=='') $_GET["type"]='devicetype';
	 if ($_GET["type"]=='devicetype') print '<h2><a class="grey" href="index.php?sel=devicetypes">Device Types</a></h2>';
	 if ($_GET["type"]=='errors') print '<h2><a class="grey" href="index.php?sel=errors">Error types</a></h2>';
	 if ($_GET["type"]=='protocols') print '<h2><a class="grey" href="index.php?sel=protocols">Protocols</a></h2>';
	 if ($_GET["type"]=='variables') print '<h2><a class="grey" href="index.php?sel=variables">Variables|Parameters</a></h2>';
	?>
	<span class="link"><img src="files/dbl_arrow.gif" alt="*" width="19" height="12"></span>
	</div>

	<div id="divFutures" class="mpbox">
	<div>
	<div class="xscroll">
	<table class="datatable_simple js" data-largetable="500" data-extrafields="" data-pagesize="100" width="100%" border="0" cellpadding="1" cellspacing="1">
	<thead><tr class="datatable_header">
	<?php	
	if ($_GET["type"]=='devicetype')
		{
		 print '<th class="ds_name sort_none" align="center">Name</th>
			<th class="ds_last sort_none" align="center">Description</th>
		        <th class="ds_change sort_none" align="center">Identificator</th>';
		}
	if ($_GET["type"]=='errors')
		{
		 print '<th class="ds_name sort_none" align="center">Id</th>
			<th class="ds_last sort_none" align="center">Code</th>
		        <th class="ds_change sort_none" align="center">Name</th>';
		}
	if ($_GET["type"]=='protocols')
		{
		 print '<th class="ds_name sort_none" align="center">Id</th>
			<th class="ds_last sort_none" align="center">Name</th>
		        <th class="ds_change sort_none" align="center">Type</th>
			<th class="ds_change sort_none" align="center">Protocol</th>';
		}
	if ($_GET["type"]=='variables')
		{
		 print '<th class="ds_name sort_none" align="center">Id</th>
			<th class="ds_last sort_none" align="center">Name</th>
		        <th class="ds_change sort_none" align="center">Prm</th>
			<th class="ds_change sort_none" align="center">Pipe</th>';
		}
	?>
	</tr></thead>
	<tbody>

	<?php
	 if ($_GET["type"]=='devicetype') $query = 'SELECT * FROM devicetype';
	 if ($_GET["type"]=='errors') $query = 'SELECT * FROM errors';
	 if ($_GET["type"]=='protocols') $query = 'SELECT * FROM protocols';
	 if ($_GET["type"]=='variables') $query = 'SELECT * FROM var2';

	 if ($a = mysql_query ($query,$i))
	 while ($uy = mysql_fetch_row ($a))
		{
		 if ($_GET["type"]=='devicetype')
			 print '<tr class=""><td class="ds_name qb_shad" align="left" nowrap="nowrap"><a href="index.php?sel=devices&type='.$uy[4].'">'.$uy[1].'</a></td>
			        <td style="" class="ds_last qb_shad" align="left">'.$uy[2].'</td>
			        <td style="" class="ds_change qb_shad" align="center" nowrap="nowrap"><span class="qb_up">'.$uy[4].'</span></td></tr>';
		 if ($_GET["type"]=='errors')
			 print '<tr class=""><td style="" class="ds_last qb_shad" align="left">'.$uy[0].'</td>
				<td class="ds_name qb_shad" align="left" nowrap="nowrap"><a href="index.php?sel=register&type='.$uy[1].'">'.$uy[2].'</a></td>
			        <td style="" class="ds_last qb_shad" align="left">'.$uy[1].'</td></tr>';
		 if ($_GET["type"]=='protocols')
			 print '<tr class=""><td style="" class="ds_last qb_shad" align="center">'.$uy[0].'</td>
				<td class="ds_name qb_shad" align="center" nowrap="nowrap">'.$uy[1].'</td>
			        <td style="" class="ds_last qb_shad" align="center">'.$uy[2].'</td>
			        <td style="" class="ds_last qb_shad" align="center">'.$uy[3].'</td></tr>';
		 if ($_GET["type"]=='variables')
			 print '<tr class=""><td style="" class="ds_last qb_shad" align="center">'.$uy[0].'</td>
				<td class="ds_name qb_shad" align="left" nowrap="nowrap">'.$uy[1].'</td>
			        <td style="" class="ds_last qb_shad" align="center">'.$uy[2].'</td>
			        <td style="" class="ds_last qb_shad" align="center">'.$uy[3].'</td></tr>';
		}
	?>
	</tbody></table>
	<div class="loading" id="ldt2"></div></div>
	</div>
	</div>
	</td>
	</tr>
</tbody></table>