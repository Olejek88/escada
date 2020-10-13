	<table class="columnLayout" style="width:940px">
	<tbody><tr>
	<td class="centercol2">
	<div class="bar firstBar">
	<?
	 $dir = '/home/user/dk/log';
	 $files = scandir($dir,SCANDIR_SORT_DESCENDING);
	 print '<h2><a class="grey" href="index.php?sel=logs">Logs</a>/'.$files[0].'</h2>';
	?>
	<span class="link"><img src="files/dbl_arrow.gif" alt="*" width="19" height="12"></span>
	</div>

	<div id="divFutures" class="mpbox">
	<div>
	<div class="xscroll">
	<table class="datatable_simple js" data-largetable="500" data-extrafields="" data-pagesize="100" width="100%" border="0" cellpadding="1" cellspacing="1">
	<thead><tr class="datatable_header">
	<th class="ds_name sort_none" align="center">rec</th>
	</tr></thead>
	<tbody>
	
	<?php
	    $handle = fopen($dir.'/'.$files[0], "r");
	    if ($handle) {
	    while (($buffer = fgets($handle, 4096)) !== false) {
	            echo '<tr><td style="font-family:calibri,tahoma; font-size:11px">'.$buffer.'</td></tr>';
	        }
	    if (!feof($handle)) {
    		echo "Error: unexpected fgets() fail\n";
	        }
	    fclose($handle);
	    }
	?>
	</tbody></table>
	</div></div>
	</div>
	</div>
	</td>
	</tr>
</tbody></table>