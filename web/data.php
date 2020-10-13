<table class="columnLayout" style="width:940px">
<tbody><tr>
<td class="maincol">
<div id="divContent" class="whitebox">

<table width="100%" border="0" cellpadding="0" cellspacing="0">
<tbody><tr>
<td style="padding-right:15px" valign="top" width="330">
<div class="bar"><h2>Data Collector Stats</h2></div>
<div class="mpbox" style="height: 257px">
<?php
$today=getdate();
$ye=$today["year"];
$mn=$today["mon"];
$day=$today["mday"];
$name='stats';
$cnt=30; $dy=31; $cn=0;
if (!checkdate ($mn,31,$ye)) { $dy=30; }
if (!checkdate ($mn,30,$ye)) { $dy=29; }
if (!checkdate ($mn,29,$ye)) { $dy=28; }

$tm=$dy=$day-1;
for ($tn=0; $tn<=$cnt; $tn++)
    {
     $dat[$tn]=sprintf ("%d%02d%02d000000",$ye,$mn,$tm);
     $date[$tn]=sprintf ("%02d",$tm);

     $query = 'SELECT COUNT(value) FROM prdata WHERE date='.$dat[$tn];
     if ($a = mysql_query ($query,$i))
     if ($uy = mysql_fetch_row ($a)) $data[$cn]=$uy[0];

     $tm--;
     if ($tm==0)
	{
	 $mn--;
	 if ($mn==0) { $mn=12; $ye--; }
	 $dy=31;
	 if (!checkdate ($mn,31,$ye)) { $dy=30; }
	 if (!checkdate ($mn,30,$ye)) { $dy=29; }
	 if (!checkdate ($mn,29,$ye)) { $dy=28; }
	 $tm=$dy;
	}
     $cn++;
    }
 include("highcharts/bar_stat.php"); 

 $query = 'SELECT COUNT(value) FROM hours';
 if ($a = mysql_query ($query,$i))
 if ($uy = mysql_fetch_row ($a)) $qd_h=$uy[0];
 $query = 'SELECT COUNT(value) FROM prdata WHERE type=0';
 if ($a = mysql_query ($query,$i))
 if ($uy = mysql_fetch_row ($a)) $qd_c=$uy[0];
 $query = 'SELECT COUNT(value) FROM prdata WHERE type=2';
 if ($a = mysql_query ($query,$i))
 if ($uy = mysql_fetch_row ($a)) $qd_d=$uy[0];
 $query = 'SELECT COUNT(value) FROM prdata WHERE type=4';
 if ($a = mysql_query ($query,$i))
 if ($uy = mysql_fetch_row ($a)) $qd_m=$uy[0];
 $query = 'SELECT COUNT(value) FROM prdata WHERE type=5';
 if ($a = mysql_query ($query,$i))
 if ($uy = mysql_fetch_row ($a)) $qd_s=$uy[0];
?>
<div id="divSnapshot">
                    
<div class="xscroll">
<table class="datatable js" width="100%" border="0" cellpadding="1" cellspacing="1">
<thead>
<tr class="datatable_header">
    <th rel="text" class="ds_name sort_none" align="center">Currents</th>
    <th rel="text" class="ds_name sort_none" align="center">Summ</th>
    <th rel="text" class="ds_name sort_none" align="center">Hours</th>
    <th rel="text" class="ds_name sort_none" align="center">Days</th>
    <th rel="text" class="ds_name sort_none" align="center">Month</th>
  </tr>
</thead><tbody>
    <tr class="">
    <td class="ds_name qb_shad" align="center" nowrap="nowrap"><?php print $qd_c; ?></td>
    <td class="ds_last qb_shad" align="center" nowrap="nowrap"><?php print $qd_s; ?></td>
    <td class="ds_last qb_shad" align="center" nowrap="nowrap"><?php print $qd_h; ?></td>
    <td class="ds_pctchange qb_shad" align="center" nowrap="nowrap"><?php print $qd_d; ?></td>
    <td class="qb_shad noprint" align="center" nowrap=""><?php print $qd_m; ?></td>
    <td class="qb_shad noprint" align="center" nowrap="nowrap"><a href="index.php?sel=devices"><img src="files/quote_icon.gif" width="12" height="10" border="0"></a>&nbsp;<a href="index.php?sel=channels"><img src="files/chart_icon.gif" width="12" height="10" border="0"></a></td>
  </tr>
</tbody></table>
<div class="loading" id="ldt1"></div></div>
</div>
</div></td>

<td valign="top">
<div class="bar"><h2><a href="/forex/ALL" class="grey">Channels per Energy Sources</a></h2>
<span class="link"><img src="files/dbl_arrow.gif" alt="" width="19" height="12"><a href="index.php?sel=channels">All channels</a></span></div>
	<div class="mpbox">
	<table width="100%" border="0" cellpadding="0" cellspacing="0">
	<tbody><tr>
	<td valign="middle" width="35"><img src="files/icon_par_m.jpg" alt="Par" title="Par" width="30""></td>
	<td valign="middle" width="140">&nbsp;<a href="index.php?sel=channels&prm=15">Пар</a></td>
	<td valign="middle" width="35"><img src="files/icon_gas_m.jpg" alt="Gas" title="Gas" width="30"></td>
	<td valign="middle" width="140">&nbsp;<a href="index.php?sel=channels&prm=13">Газ</a></td>
	<td valign="middle"><img src="files/icon_heat_m.jpg" alt="Heat" title="Heat" width="30"></td>
	<td valign="middle" width="140">&nbsp;<a href="index.php?sel=channels&prm=13">Тепло</a></td>
	</tr>
	<tr>
	<td valign="middle"><img src="files/icon_electricity_m.jpg" alt="Electricity" title="Electricity" width="30"></td>
	<td valign="middle" width="140">&nbsp;<a href="index.php?sel=channels&prm=14">Электричество</a></td>
	<td valign="middle"><img src="files/icon_water_m.jpg" alt="Water" title="Water" width="30"></td>
	<td valign="middle" width="140">&nbsp;<a href="index.php?sel=channels&prm=12">Вода</a></td>
	<td valign="middle" width="35"><img src="" alt="" title="" width="30"></td>
	<td valign="middle" width="140">&nbsp;<a href=""></a></td>
	</tr>
	</tbody></table>
	</div>

	<div class="bar"><h2>Data Sructure</h2></div>
	<div id="divHeatMap" class="mpbox">
	<?php
	 include("highcharts/combo_info.php"); 
	?>
	</div>
	</td>
	</tr>
	<tr>
	<td colspan="2"></td>
	</tr>
  </tbody></table>

<div class="halfbreak"></div>
<div class="bar"><div class="fl"><h2>Channels</h2></div><span class="link"></div>
<div id="divBigTable" class="mpbox"><div class="halfbreak"></div>
<div class="xscroll">
<table class="datatable js" id="dt2" width="100%" border="0" cellpadding="1" cellspacing="1">
<thead>
<tr class="datatable_header" id="dt2_column">
<th rel="text" class="ds_symbol sort_none" align="left">Id</th>
    <th rel="text" class="ds_name sort_none" align="center">Name</th>
    <th rel="change" class="ds_change sort_none" align="center">Curr</th>
    <th rel="price" class="ds_last sort_none" align="center">Nak</th>
    <th rel="change" class="ds_change sort_none" align="center">Hour</th>
    <th rel="change" class="ds_change sort_none" align="center">Ch(%)</th>
    <th rel="change" class="ds_change sort_none" align="center">Low</th>
    <th rel="change" class="ds_change sort_none" align="center">High</th>
    <th rel="change" class="ds_change sort_none" align="center">Day</th>
    <th rel="change" class="ds_change sort_none" align="center">Ch(%)</th>
    <th rel="change" class="ds_change sort_none" align="center">Low</th>
    <th rel="change" class="ds_change sort_none" align="center">High</th>
    <th rel="change" class="ds_change sort_none" align="center">Month</th>
    <th rel="change" class="ds_change sort_none" align="center">High</th>
  </tr>
</thead><tbody> 
<tr class="">
<?php
    $query = 'SELECT * FROM channels ORDER BY RAND() LIMIT 10'; $cn=0;
    if ($a = mysql_query ($query,$i))
    while ($uy = mysql_fetch_row ($a))
	{
	 $query = 'SELECT * FROM device WHERE idd='.$uy[2];
	 if ($a2 = mysql_query ($query,$i))
	 if ($uy2 = mysql_fetch_row ($a2)) $devname=$uy2[20];
	 $query = 'SELECT * FROM var2 WHERE prm='.$uy[9].' AND pipe='.$uy[10];
 	 if ($a2 = mysql_query ($query,$i))
	 if ($uy2 = mysql_fetch_row ($a2)) $var=$uy2[1];

	 $query = 'SELECT MAX(value),MIN(value) FROM hours WHERE channel='.$uy[0];
	 if ($e4 = mysql_query ($query,$i))
	 if  ($uo2 = mysql_fetch_row ($e4)) { $max=$uo2[0]; $min=$uo2[1]; }
	 $query = 'SELECT MAX(value),MIN(value) FROM data WHERE type=2 AND channel='.$uy[0];
	 if ($e4 = mysql_query ($query,$i))
	 if  ($uo2 = mysql_fetch_row ($e4)) { $max2=$uo2[0]; $min2=$uo2[1]; }
	 $query = 'SELECT MAX(value),MIN(value) FROM data WHERE type=4 AND channel='.$uy[0];
	 if ($e4 = mysql_query ($query,$i))
	 if  ($uo2 = mysql_fetch_row ($e4)) { $max3=$uo2[0]; $min3=$uo2[1]; }

	 $last=$plast=$change=$value=$dvalue=$dvalue2=$mvalue=$mvalue2=$pvalue=0;
	 $query = 'SELECT * FROM hours WHERE channel='.$uy[0].' ORDER BY date DESC LIMIT 2';
	 if ($e4 = mysql_query ($query,$i))
	 if ($uo2 = mysql_fetch_row ($e4))
		{
		 $last=$uo2[5];
		 $uo2 = mysql_fetch_row ($e4);
		 $plast=$uo2[5];
		 if ($last) $change=number_format(($last-$plast)*100/$last,2);
		 else $change=0;
		}

	 $query = 'SELECT * FROM prdata WHERE channel='.$uy[0].' ORDER BY date DESC';
	 if ($e4 = mysql_query ($query,$i))
	 while ($uo2 = mysql_fetch_row ($e4))
		{
		 if ($uo2[2]==0) { $value=$uo2[5]; $time=$uo2[4]; }
		 if ($uo2[2]==2 && !$dvalue) $dvalue=$uo2[5];
		 if ($uo2[2]==2 && $dvalue && !$dvalue2) $dvalue2=$uo2[5];
		 if ($uo2[2]==4 && !$mvalue) $mvalue=$uo2[5];
		 if ($uo2[2]==4 && $mvalue && !$mvalue2) $mvalue2=$uo2[5];
		}
	 if ($dvalue) $change2=number_format(($dvalue-$dvalue2)*100/$dvalue,3);
	 else $change2=0;

	 $query = 'SELECT value FROM prdata WHERE type=5 AND channel='.$uy[0].' ORDER BY date DESC';
	 if ($e4 = mysql_query ($query,$i))
	 if ($uo2 = mysql_fetch_row ($e4))
		 $pvalue=$uo2[0]; 

	 if ($cn%2==0) $style='qb_shad'; else $style='qb_line';
	 print '<td class="ds_symbol '.$style.'" align="left" nowrap="nowrap">'.$uy[0].'</td>';	 
	 print '<td class="ds_name '.$style.'" align="left" nowrap="nowrap"><a href="index.php?sel=channel&id='.$uy[0].'">'.substr($uy[1],0,40).'</a></td>';
	 //print '<td class="ds_displaytime '.$style.'" align="right" nowrap="nowrap">'.$time.'</td>';
	 print '<td class="ds_high '.$style.'" align="right" nowrap="nowrap">'.number_format($value,4).'</td>
		<td class="ds_high '.$style.'" align="right" nowrap="nowrap">'.number_format($pvalue,2).'</td>
		<td class="ds_high '.$style.'" align="right" nowrap="nowrap">'.number_format($last,4).'</td>';
	 if ($change>0) print '<td style="background-color: rgb(255, 255, 161);" class="ds_pctchange qb_shad" align="right" nowrap="nowrap"><span class="qb_up">'.$change.'</span></td>';
	 else if ($change<0) print '<td style="background-color: rgb(255, 255, 161);" class="ds_pctchange qb_shad" align="right" nowrap="nowrap"><span class="qb_down">'.$change.'</span></td>';
	 else print '<td style="background-color: rgb(255, 255, 161);" class="ds_pctchange qb_shad" align="right" nowrap="nowrap">0</td>';

	 print '<td class="ds_pctchange '.$style.'" align="right" nowrap="nowrap"><span class="qb_down">'.number_format($min,4).'</span></td>';

	 if ($max<100) print '<td class="ds_pctchange '.$style.'" align="right" nowrap="nowrap"><span class="qb_up">'.number_format($max,4).'</span></td>';
	 else if ($max<1000) print '<td class="ds_pctchange '.$style.'" align="right" nowrap="nowrap"><span class="qb_up">'.number_format($max,3).'</span></td>';
	 else print '<td class="ds_pctchange '.$style.'" align="right" nowrap="nowrap"><span class="qb_up">'.number_format($max,1).'</span></td>';

	 if ($dvalue<100) print '<td class="ds_high '.$style.'" align="right" nowrap="nowrap">'.number_format($dvalue,3).'</td>';
	 else print '<td class="ds_high '.$style.'" align="right" nowrap="nowrap">'.number_format($dvalue,1).'</td>';
	 print '<td style="background-color: rgb(255, 255, 161);" class="ds_high qb_shad" align="right" nowrap="nowrap">'.$change2.'</td>';

	 print '<td class="ds_pctchange '.$style.'" align="right" nowrap="nowrap"><span class="qb_down">'.number_format($min2,4).'</span></td>';
	 print '<td class="ds_pctchange '.$style.'" align="right" nowrap="nowrap"><span class="qb_up">'.number_format($max2,4).'</span></td>';

	 print '<td class="ds_high '.$style.'" align="right" nowrap="nowrap">'.number_format($mvalue,2).'</td>';
	 if ($max3<1000) print '<td class="ds_pctchange  '.$style.'" align="right" nowrap="nowrap"><span class="qb_up">'.$max3.'</span></td>';
	 else print '<td class="ds_pctchange  '.$style.'" align="right" nowrap="nowrap"><span class="qb_up">'.number_format($max3,2).'</span></td>';
	 print '</tr>';
	 $cn++;
	}
?>
</tbody></table>
<div class="loading" id="ldt2"></div></div>
</div><div class="break"></div>
</td>
</tr></table>