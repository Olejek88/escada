	<table class="columnLayout" style="width:630px">
	<tbody><tr>
	<td class="centercol" style="width:630px">

		<div class="bar firstBar" style="width:620px">
		<h2><a class="grey" href="index.php?sel=events">Events</a></h2>
		<span class="link"><img src="files/dbl_arrow.gif" alt="*" width="19" height="12"><a href="index.php?sel=events">More Events</a></span>
		</div>

		<div class="mpbox">
		<ul class="news_headlines">
		<?php
		 $query = 'SELECT * FROM register ORDER BY id DESC LIMIT 5';
		 if ($a = mysql_query ($query,$i))
		 while ($uy = mysql_fetch_row ($a))
			  print '<li><a href="index.php?sel=events">'.$uy[3].' - level '.$uy[6].' ['.$uy[2].']</a> <span class="byline"> - '.$uy[5].'</span></li>';
		?>
		</ul></div>

		<div class="bar" style="width:620px">
		<h2><a href="index.php?sel=channels" class="grey">Channels</a></h2>
		<span class="link"><img src="files/dbl_arrow.gif" alt="*" width="19" height="12"><a href="index.php?sel=channels">View All Channels</a></span>
		</div>

		<div id="divFutures" class="mpbox" style="width:610px">
			<div>
			<div class="xscroll">
			<table class="datatable_simple js" data-largetable="500" data-extrafields="" data-pagesize="100" width="100%" border="0" cellpadding="1" cellspacing="1">
			<thead><tr class="datatable_header" id="dt2_column">
			<th rel="text" id="dt12_column_name" class="ds_name sort_none" align="left">Name</th>
			<th rel="price" id="dt12_column_last" class="ds_last sort_none" align="right">Current</th>
		        <th rel="change" id="dt12_column_change" class="ds_change sort_none" align="right">Hours</th>
			<th rel="opinion" id="dt12_column_opinion.op" class="ds_opinion--op sort_none" align="right">Days</th>
			</tr></thead>
			<tbody>

			<?php
			 $query = 'SELECT * FROM channels ORDER BY id DESC LIMIT 10';
			 if ($a = mysql_query ($query,$i))
			 while ($uy = mysql_fetch_row ($a))
				{
				 $chan=$uy[0]; $device=$uy[2]; $prm=$uy[9]; $source=$uy[10];
				 //$query = 'SELECT * FROM data WHERE prm='.$prm.' AND source='.$source.' AND channel='.$chan;
				 $query = 'SELECT * FROM prdata WHERE type=0 AND channel='.$chan.' ORDER BY date DESC';
				 if ($r = mysql_query ($query,$i))
				 if ($ui = mysql_fetch_row ($r)) { $date=$ui[4]; $value1=$ui[5]; }
				 $query = 'SELECT * FROM prdata WHERE type=2 AND channel='.$chan.' ORDER BY date DESC';
				 if ($r = mysql_query ($query,$i))
				 if ($ui = mysql_fetch_row ($r)) { $date_days=$ui[4]; $value2=$ui[5]; }
				 $query = 'SELECT * FROM hours WHERE type=1 AND channel='.$chan.' ORDER BY date DESC';
				 if ($r = mysql_query ($query,$i))
				 if ($ui = mysql_fetch_row ($r)) { $date_hours=$ui[4]; $value3=$ui[5]; }

				 print '<tr id="dt2_GCQ3" class="">
					<td class="ds_name qb_shad" align="left" nowrap="nowrap"><a href="index.php?sel=channel&id='.$uy[0].'">'.$uy[1].'</a></td>
				        <td style="" class="ds_last qb_shad" align="right" nowrap="nowrap">'.$value1.'</td>
				        <td style="" class="ds_change qb_shad" align="right" nowrap="nowrap"><span class="qb_up">'.$value2.'</span></td>
				        <td class="ds_opinion--op qb_shad" align="right" nowrap="nowrap"><span class="qb_down">'.$value3.'</span></td>
					</tr>';
				}
			?>
			</tbody></table>
		<div class="loading" id="ldt2"></div></div>
		</div>
		</div>

                <div class="bar" style="width:620px">
	        <h2><a class="grey" href="index.php?sel=devices">Devices</a></h2>
                <span class="link"><img src="files/dbl_arrow.gif" alt="*" width="19" height="12"><a href="index.php?sel=devices">View All Devices</a></span>
	        </div>
	        <div id="divTop100" class="mpbox" style="width:610px">
		<div class="xscroll">
		<table class="datatable_simple js" id="dt3" data-largetable="500" data-extrafields="" data-pagesize="5" width="100%" border="0" cellpadding="1" cellspacing="1">
		<thead>
		<tr class="datatable_header">
		<th rel="text" class="ds_name sort_none" align="left">Device</th>
	        <th rel="change" class="ds_weighted_alpha sort_disabled" align="right">Port</th>
		<th rel="price" id="dt13_column_last" class="ds_last sort_none" align="right">Speed</th>
		<th rel="pctchange" id="dt13_column_pctchange" class="ds_pctchange sort_none" align="right">Addr</th>
		<th rel="opinion" id="dt13_column_opinion.op" class="ds_opinion--op sort_none" align="right">Last update</th>
		</tr></thead>
		<tbody>
		<?php
		 $query = 'SELECT * FROM device ORDER BY lastdate DESC LIMIT 10';
		 if ($a = mysql_query ($query,$i))
		 while ($uy = mysql_fetch_row ($a))
			{
			 print '<tr id="dt3_NXST" class="">
			        <td class="ds_name qb_shad" align="left" nowrap="nowrap"><a href="index.php?sel=channels&device='.$ui[1].'">'.$uy[20].' ['.$uy[1].']</a></td>';
			 $query = 'SELECT * FROM devicetype WHERE ids='.$uy[8];
			 if ($r = mysql_query ($query,$i))
			 if ($ui = mysql_fetch_row ($r)) { $type=$ui[1]; }
			 if ($uy[5]!=255)
			    print '<td class="ds_weighted_alpha qb_shad" align="right" nowrap="nowrap"><span class="qb_up">/dev/ttyS'.$uy[5].'</span></td>';
			 else print '<td class="ds_weighted_alpha qb_shad" align="right" nowrap="nowrap"><span class="qb_up">eth:'.$uy[9].'</span></td>';
			 print '<td class="ds_last qb_shad" align="right" nowrap="nowrap">'.$uy[6].'</td>
				<td class="ds_pctchange qb_shad" align="right" nowrap="nowrap"><span class="qb_up">'.$uy[7].'</span></td>
				<td class="ds_opinion--op qb_shad" align="right" nowrap="nowrap"><span class="qb_up">'.$uy[16].'</span></td>
				</tr>';
			}

		 $query = 'SELECT * FROM devices ORDER BY id DESC LIMIT 10';
		 if ($a = mysql_query ($query,$i))
		 while ($uy = mysql_fetch_row ($a))
			{
			 print '<tr id="dt3_NXST" class="">
			        <td class="ds_name qb_shad" align="left" nowrap="nowrap"><a href="index.php?sel=device">'.$uy[1].' ['.$uy[11].']</a></td>';
			 $query = 'SELECT * FROM devicetype WHERE ids='.$uy[8];
			 if ($r = mysql_query ($query,$i))
			 if ($ui = mysql_fetch_row ($r)) { $type=$ui[1]; }
			 if ($uy[4]!=255)
			    print '<td class="ds_weighted_alpha qb_shad" align="right" nowrap="nowrap"><span class="qb_up">/dev/ttyS'.$uy[4].'</span></td>';
			 else print '<td class="ds_weighted_alpha qb_shad" align="right" nowrap="nowrap"><span class="qb_up">-</span></td>';
			 print '<td class="ds_last qb_shad" align="right" nowrap="nowrap">'.$uy[3].'</td>
				<td class="ds_pctchange qb_shad" align="right" nowrap="nowrap"><span class="qb_up">'.$uy[10].'</span></td>
				<td class="ds_opinion--op qb_shad" align="right" nowrap="nowrap"><span class="qb_up">'.$uy[7].'</span></td>
				</tr>';
			}
		?>
		</tbody></table>
		<div class="loading" id="ldt3"></div></div>
	        </div>
	</td>

	<td class="rightcol" style="width:300px;">
		<div class="bar firstBar"><h2>Object info</h2></div>

		<div class="mpbox">
		<?php
		 $query = 'SELECT * FROM build';
		 if ($a = mysql_query ($query,$i))
		 if ($uy = mysql_fetch_row ($a))
			  print '<b>Название: '.$uy[1].'<br>Описание:</b> <span class="byline">'.$uy[5].'</span>';
		?>
		</div>
		<div class="loading" id="ldt1"><br></div>

		<div class="bar break" style="300px">
		<h2><a href="" class="grey">Controller status</a></h2>
		</div>
		<div id="divLeaders" class="mpbox">
		<div class="xscroll">
		<?php
		 $query = 'SELECT * FROM info';
		 if ($a = mysql_query ($query,$i))
		 if ($uy = mysql_fetch_row ($a))
			{
			 $date=$uy[1]; $log=$uy[2]; $time=$uy[3]; $linux=$uy[4];
			 $hardware=$uy[5]; $base=$uy[6]; $soft=$uy[7]; $ip=$uy[8];
			}
		?>
		<table class="datatable js" width="300px" border="0" cellpadding="1" cellspacing="1">
		<tbody>
		<tr class=""><td class="ds_symbol qb_shad" align="left" nowrap="nowrap">status:</td>
		<?php
		 $today=getdate(); 
		 $dat=$date[4]*1000+$date[5]*100+$date[6]*10+$date[7];
		 $dat2=$today["mon"]*100+$today["mday"];
		 if ($dat2==$dat) print '<td class="ds_pctchange qb_line" align="right" nowrap="nowrap"><span class="qb_up">running</span></td>';
		 else print '<td class="ds_pctchange qb_shad" align="left" nowrap="nowrap"><span class="qb_down">stopped</span></td>';
		?>
		</tr>
		<tr class=""><td class="ds_symbol qb_shad" align="left" nowrap="nowrap">current log</td>
		<td class="ds_pctchange qb_line" align="left" nowrap="nowrap"><?php print $log; ?></td></tr>
		<tr class=""><td class="ds_symbol qb_shad" align="left" nowrap="nowrap">controller time</td>
		<td class="ds_pctchange qb_line" align="left" nowrap="nowrap"><?php print $time; ?></td></tr>
		<tr class=""><td class="ds_symbol qb_shad" align="left" nowrap="nowrap">linux version</td>
		<td class="ds_pctchange qb_line" align="left" nowrap="nowrap"><?php print $linux; ?></td></tr>
		<tr class=""><td class="ds_symbol qb_shad" align="left" nowrap="nowrap">hardware</td>
		<td class="ds_pctchange qb_line" align="left" nowrap="nowrap"><?php print $hardware; ?></td></tr>
		<tr class=""><td class="ds_symbol qb_shad" align="left" nowrap="nowrap">database</td>
		<td class="ds_pctchange qb_line" align="left" nowrap="nowrap"><?php print $base; ?></td></tr>
		<tr class=""><td class="ds_symbol qb_shad" align="left" nowrap="nowrap">software</td>
		<td class="ds_pctchange qb_line" align="left" nowrap="nowrap"><?php print $soft; ?></td></tr>
		<tr class=""><td class="ds_symbol qb_shad" align="left" nowrap="nowrap">ip address</td>
		<td class="ds_pctchange qb_line" align="left" nowrap="nowrap"><?php print $ip; ?></td></tr>

		</tbody></table>
		<div class="loading" id="ldt2"></div></div>
		</div>
		<?php include ("cpu3.php"); ?>
		<div class="bar break" style="300px">
		<h2><a href="" class="grey">Devices threads status</a></h2>
		</div>
		<div id="divLeaders" class="mpbox">
		<div class="xscroll">
		<table class="datatable js" width="300px" border="0" cellpadding="1" cellspacing="1">
		<thead><tr class="datatable_header" id="dt2_column">
		<th rel="text" class="ds_symbol sort_none" align="center">IDs</th>
		<th rel="price" class="ds_last sort_none" align="center">Name</th>
		<th rel="pctchange" class="ds_pctchange sort_none" align="center">Qnt</th>
		<th class="ds_links sort_disabled noprint" align="center">Status</th>
		<th class="ds_pctchange sort_none" align="center"></th>
		</tr>
		</thead><tbody>
		<?php
		 $query = 'SELECT * FROM devicetype';
		 if ($a = mysql_query ($query,$i))
		 while ($uy = mysql_fetch_row ($a))
			{
			 $query = 'SELECT COUNT(id) FROM device WHERE type='.$uy[4];
			 if ($r = mysql_query ($query,$i))
			 if ($ur = mysql_fetch_row ($r)) $cnt=$ur[0];
			 $query = 'SELECT pth_'.($uy[4]-1).' FROM dev_dk';
			 if ($r = mysql_query ($query,$i))
			 if ($ur = mysql_fetch_row ($r)) $act=$ur[0];
			 if ($cnt>0)
				{
				 print '<tr class=""><td class="ds_symbol qb_shad" align="center" nowrap="nowrap">'.$uy[4].'</td>
					<td class="ds_last qb_shad" align="left" nowrap="nowrap">'.$uy[5].'</td>
					<td class="ds_pctchange qb_shad" align="center" nowrap="nowrap"><span class="qb_down">'.$cnt.'</span></td>';
				 if ($act) print '<td class="ds_pctchange qb_shad" align="center" nowrap="nowrap"><span class="qb_up">active</span></td>';
				 else  print '<td class="ds_pctchange qb_shad" align="center" nowrap="nowrap"><span class="qb_down">stopped</span></td>';
				 print '<td class="qb_shad noprint" align="center" nowrap="nowrap"><a href="index.php?sel=devices&type='.$uy[4].'"><img src="files/quote_icon.gif" width="12" height="10" border="0"></a>&nbsp;<a href="index.php?sel=channels&devicetype='.$uy[4].'"><img src="files/chart_icon.gif" width="12" height="10" border="0"></a></td></tr>';
				}
			}
		?>
	</tbody></table>
	<div class="loading" id="ldt2"></div></div>
	</div>
	</td>
	</tr>
</tbody></table>