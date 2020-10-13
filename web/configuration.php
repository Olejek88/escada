<?php
 if ($_POST["conf"]=='1')
	{
	 //phpinfo ();
	 $query = 'SELECT * FROM devicetype';
	 if ($r = mysql_query ($query,$i))
	 while ($ur = mysql_fetch_row ($r)) 
		{ 
		 $pth_start=0;
		 $pthh='start'.$ur[4];
		 if ($_POST[$pthh]=='on') $pth_start=1;
		 $query = 'UPDATE dev_dk SET pth_'.$ur[4].'='.$pth_start;
		 //echo $query.'<br>';
		 mysql_query ($query,$i);
        	} 
	}
 if ($_POST["conf"]=='2')
	{
	 $formxml=0;
	 $crqenable=0;
	 $debug=0;
	 $pt1='formxml';
	 $pt2='crqenable';
	 $pt3='debug';
	 if ($_POST[$pt1]=='on') $formxml=1;
	 if ($_POST[$pt2]=='on') $crqenable=1;
	 $debug=$_POST["debug"];
	 $query = 'UPDATE dev_dk SET formxml='.$formxml.', crq_enabl='.$crqenable.', log='.$debug;
	 //echo $query.'<br>';
	 mysql_query ($query,$i);
	}
 if ($_POST["conf"]=='3')
	{
	 $query = 'UPDATE dev_dk SET regim=0';
	 mysql_query ($query,$i);
	}
?>
	<table class="columnLayout" style="width:630px">
	<tbody><tr>
	<td class="centercol" style="width:630px">

		<div class="bar" style="width:620px"><h2><a href="index.php?sel=channels" class="grey">Drivers & Threads</a></h2></div>
		<div id="divLeaders" class="mpbox">
		<div class="xscroll">
		<form name="frm1" method="post" action="index.php?sel=configuration">
		<table class="datatable js" width="300px" border="0" cellpadding="3" cellspacing="1">
		<tbody>
		<tr class=""><td class="ds_symbol qb_shad" align="center" nowrap="nowrap" style="width:20px">id</td>
		<td class="ds_symbol qb_shad" align="center" nowrap="nowrap">Device</td>
		<td class="ds_symbol qb_shad" align="center" nowrap="nowrap">Filename</td>
		<td class="ds_symbol qb_shad" align="center" nowrap="nowrap">Global</td>
		<td class="ds_symbol qb_shad" align="center" nowrap="nowrap">cAddr</td>
		<td class="ds_symbol qb_shad" align="center" nowrap="nowrap">cNum</td>
		<td class="ds_symbol qb_shad" align="center" nowrap="nowrap">TimeStamp</td>
		<td class="ds_symbol qb_shad" align="center" nowrap="nowrap">Qnt</td>
		<td class="ds_symbol qb_shad" align="center" nowrap="nowrap">Status</td>
		<td class="ds_symbol qb_shad" align="center" nowrap="nowrap">Type</td>
		<td class="ds_symbol qb_shad" align="center" nowrap="nowrap">Start</td>
		</tr>
		<?php
		 $query = 'SELECT * FROM threads';
		 if ($a = mysql_query ($query,$i))
		 while ($uy = mysql_fetch_row ($a))
			{
			 $query = 'SELECT * FROM devicetype WHERE ids='.($uy[1]+1);
			 if ($r = mysql_query ($query,$i))
			 if ($ur = mysql_fetch_row ($r)) { $idtype=$ur[4]; $device=$ur[5]; $lib=$ur[7]; } 
			 $query = 'SELECT COUNT(id) FROM device WHERE type='.($uy[1]+1);
			 if ($r = mysql_query ($query,$i))
			 if ($ur = mysql_fetch_row ($r)) { $qnt=$ur[0]; } 
			 $query = 'SELECT pth_'.$uy[1].' FROM dev_dk';
			 if ($r = mysql_query ($query,$i))
			 if ($ur = mysql_fetch_row ($r)) $pth=$ur[0];  

			 if ($lib!='')
				{
				 print '<tr class=""><td class="ds_symbol qb_shad" align="center" nowrap="nowrap" style="width:20px">&nbsp;'.$idtype.'&nbsp;</td>
					<td class="ds_last qb_line" align="left" nowrap="nowrap">'.$device.'</td>
					<td class="ds_pctchange qb_line" align="left" nowrap="nowrap">'.$lib.'</td>';
				 if ($uy[2]) print '<td class="ds_pctchange qb_shad" align="center" nowrap="nowrap"><span class="qb_up">active</span></td>';
				 else  print '<td class="ds_pctchange qb_shad" align="center" nowrap="nowrap"><span class="qb_down">stopped</span></td>';
				 print '<td class="ds_last qb_line" align="center" nowrap="nowrap">'.$uy[5].'</td>';
				 print '<td class="ds_last qb_line" align="center" nowrap="nowrap">'.$uy[4].'</td>';
				 print '<td class="ds_last qb_line" align="center" nowrap="nowrap">'.$uy[8].'</td>';
				 print '<td class="ds_last qb_line" align="center" nowrap="nowrap">'.$qnt.'</td>';
				 if ($pth) print '<td class="ds_pctchange qb_line" align="center" nowrap="nowrap"><span class="qb_up">start</span></td>';
				 else  print '<td class="ds_pctchange qb_line" align="center" nowrap="nowrap"><span class="qb_down">stop</span></td>';
				 print '<td class="ds_last qb_line" align="left" nowrap="nowrap">'.$uy[7].'</td>';
				 print '<td class="qb_shad noprint" align="center" nowrap="nowrap"><input type="checkbox" id="start'.$uy[1].'" name="start'.$uy[1].'" ';
				 if ($pth) print 'checked';
				 print '></td></tr>';
				}
			}
		?>
		</tbody></table>
		<table><tr><td><input name="conf" type="submit" value="change" style="font-size:10px"><input name="conf" size=1 style="height:1;width:1;visibility:hidden" value="1"></td></tr></table>
		</form>
		<div class="loading" id="ldt2"></div></div>
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
		 $query = 'SELECT * FROM device ORDER BY id DESC LIMIT 15';
		 if ($a = mysql_query ($query,$i))
		 while ($uy = mysql_fetch_row ($a))
			{
			 print '<tr id="dt3_NXST" class="">
			        <td class="ds_name qb_shad" align="left" nowrap="nowrap"><a href="index.php?sel=device">'.$uy[20].' ['.$uy[1].']</a></td>';
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

		 $query = 'SELECT * FROM devices ORDER BY id DESC LIMIT 15';
		 if ($a = mysql_query ($query,$i))
		 while ($uy = mysql_fetch_row ($a))
			{
			 print '<tr id="dt3_NXST" class="">
			        <td class="ds_name qb_shad" align="left" nowrap="nowrap"><a href="index.php?sel=device">'.$uy[1].' ['.$uy[11].']</a></td>';
			 $query = 'SELECT * FROM devicetype WHERE ids='.$uy[8];
			 if ($r = mysql_query ($query,$i))
			 if ($ui = mysql_fetch_row ($r)) { $type=$ui[1]; }
			 print '<td class="ds_weighted_alpha qb_shad" align="right" nowrap="nowrap"><span class="qb_up">'.$uy[4].'</span></td>
				<td class="ds_last qb_shad" align="right" nowrap="nowrap">'.$uy[3].'</td>
				<td class="ds_pctchange qb_shad" align="right" nowrap="nowrap"><span class="qb_up">'.$uy[10].'</span></td>
				<td class="ds_opinion--op qb_shad" align="right" nowrap="nowrap"><span class="qb_up">'.$uy[7].'</span></td>
				</tr>';
			}
		?>
		</tbody></table>
		<div class="loading" id="ldt3"></div></div>
	        </div>

                <div class="bar" style="width:620px">
	        <h2>Memory usage</h2></div>
	        <div id="divTop100" class="mpbox" style="width:610px">
		<div class="xscroll">
		<?php include ("mem.php"); ?>
		</div>
		</div>
	</td>
	<td class="rightcol" style="width:300px;">
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
		 $dat=$date[5]*10000000+$date[6]*1000000+$date[8]*100000+$date[9]*10000+$date[11]*1000+$date[12]*100+$date[14]*10+$date[15];
		 $dat2=$today["mon"]*1000000+$today["mday"]*10000+$today["hours"]*100+$today["minutes"];
		 //echo $dat2.'='.$dat;
		 if ($dat2>$dat+1) print '<td class="ds_pctchange qb_line" align="left" nowrap="nowrap"><span class="qb_down">stopped</span></td>';
		 else print '<td class="ds_pctchange qb_shad" align="left" nowrap="nowrap"><span class="qb_up">running</span></td>';
		?>
		</tr>
		<tr class=""><td class="ds_symbol qb_shad" align="left" nowrap="nowrap">current log</td>
		<td class="ds_pctchange qb_line" align="left" nowrap="nowrap"><?php print $log; ?></td></tr>
		<tr class=""><td class="ds_symbol qb_shad" align="left" nowrap="nowrap">controller time</td>
		<td class="ds_pctchange qb_line" align="left" nowrap="nowrap"><?php print $date; ?></td></tr>
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
		<h2><a href="index.php?sel=configuration" class="grey">Controller config</a></h2>
		</div>
		<div id="divLeaders" class="mpbox">
		<div class="xscroll">
		<form name="frm1" method="post" action="index.php?sel=configuration">
		<table class="datatable js" width="300px" border="0" cellpadding="1" cellspacing="1">
		<tbody>
		<tr class=""><td class="ds_symbol qb_shad" align="left" nowrap="nowrap">&nbsp;debug</td>
		<td class="ds_pctchange qb_line" align="left" nowrap="nowrap" valign="center">
		<select class=log id="debug" name="debug" style="height:14; font-size:10px">
		<?php
		 $query = 'SELECT * FROM dev_dk';
		 if ($a = mysql_query ($query,$i))
		    $uy = mysql_fetch_row ($a);
		?>
		<option value="0" <?php if ($uy[15]==0) print 'selected';?>>[0] ничего не выводить
		<option value="1" <?php if ($uy[15]==1) print 'selected';?>>[1] только важна€
		<option value="2" <?php if ($uy[15]==2) print 'selected';?>>[2] основна€ информаци€
		<option value="3" <?php if ($uy[15]==3) print 'selected';?>>[3] все кроме тех. информации
		<option value="4" <?php if ($uy[15]==4) print 'selected';?>>[4] выводить все
		</select>
		</td></tr>
		<tr class=""><td class="ds_symbol qb_shad" align="left" nowrap="nowrap">&nbsp;form xml</td>
		<?php
    			print '<td><input type="checkbox" id="formxml" name="formxml" ';
		        if ($uy[37]) print 'checked';
			print '></td>';
		?>
		<tr class=""><td class="ds_symbol qb_shad" align="left" nowrap="nowrap">&nbsp;crq enable</td>
		<?php
    			print '<td><input type="checkbox" id="crqenable" name="crqenable" ';
		        if ($uy[38]) print 'checked';
			print '></td>';
		?>
		</tr>
		</tbody></table>
		<table><tr><td><input name="conf" type="submit" value="change" style="font-size:10px"><input name="conf" size=1 style="height:1;width:1;visibility:hidden" value="2"></td></tr></table>
		</form>
		<form name="frm1" method="post" action="index.php?sel=configuration">
		<table><tr><td><input name="conf" type="submit" value="stop controller" style="font-size:10px"><input name="conf" size=1 style="height:1;width:1;visibility:hidden" value="3"></td></tr></table>
		</form>
		<div class="loading" id="ldt2"></div></div>
		</div>
	</div>
	</td>
	</tr>
</tbody></table>