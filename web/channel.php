<table class="columnLayout" style="width:940px">
<tbody><tr>
<td class="centercol2">
<div class="bar firstBar">
<?php
 $query = 'SELECT * FROM channels WHERE id='.$_GET["id"];
 if ($e) $e = mysql_query ($query,$i);
 while ($ui = mysql_fetch_row ($e))
	{ 
	 $chan=$ui[0]; $name=$ui[1]; $sname=$ui[15];

	 $query = 'SELECT * FROM device WHERE idd='.$ui[2];
	 if ($a2 = mysql_query ($query,$i))
	 if ($uy2 = mysql_fetch_row ($a2))
	    {
	     $dev=$uy2[20];
	    }
	}
 print '<h2><a class="grey" href="index.php?sel=devices">'.$dev.'</a> / '.$name.'</h2>';
?>
</div>

<div id="divFutures" class="mpbox">
<div>
<div class="xscroll">

<table class="datatable_simple js" border="0" cellpadding="0" cellspacing="0" width="600px">
<tr class="datatable_header">
<td width="200px">
<table width=200 bgcolor=#eeeeee valign=top cellpadding=2 cellspacing=2>
<?php
 print '<tr class="BlockHeaderLeftRight" style="height:18px"><td bgcolor="#1881b6" align=center style="font-family: Verdana; font-size: 11px; color:white">дата</td>';
 print '<td bgcolor="#1881b6" align="center" style="font-family: Verdana; font-size: 11px; color:white">'.$sname.'</td></tr>';

 $today=getdate(); $cnt=500;
 if ($_GET["year"]=='') $ye=$today["year"];
 else $ye=$_GET["year"];
 if ($_GET["month"]=='') $mn=$today["mon"];
 else $mn=$_GET["month"];
 $x=0; $nn=1; $ts=$today["hours"];
 $tm=$dy=$today["mday"];

 for ($tn=0; $tn<=$cnt; $tn++)
	{
	 $dat[$tn]=sprintf ("%d-%02d-%02d %02d:00:00",$ye,$mn,$tm,$ts);
	 $date1[$tn]=sprintf ("%02d-%02d",$mn,$tm);

         $x++;// $tm--;
	 if ($tm==1 && $ts==0)
		{
		 $mn--; $ts=24;	
		 $dy=31;
		 if (!checkdate ($mn,31,$ye)) { $dy=30; }
		 if (!checkdate ($mn,30,$ye)) { $dy=29; }
		 if (!checkdate ($mn,29,$ye)) { $dy=28; }
		 $tm=$dy;
		}
 	 if ($ts==0) { $ts=24; $tm--; }
	 $ts--;
       }

 $query = 'SELECT * FROM hours WHERE type=9 AND channel='.$_GET["id"].' ORDER BY date DESC';
 $x=0;                                                            
//echo $query;
 if ($a = mysql_query ($query,$i))
 while ($uy = mysql_fetch_row ($a))
	{
	 $date1[$x]=$uy[4][11].$uy[4][15];
         $data[$x]=number_format($uy[5],3); $x++;
        }

 $cn=0;
 for ($tn=0; $tn<=$cnt; $tn++)
 if ($data[$tn])
	{
	 print '<tr><td align=center bgcolor="#1881b6"><font style="font-family: Verdana; font-size: 11px; color:white">'.$dat[$tn].'</font></td>';
	 print '<td align=center bgcolor=#ffffff class="simple">'.$data[$tn].'</td>';
	 print '</td></tr>';
	 $cn++; if ($cn>10) break;
	}
print '</table></td>';

print '<td valign="top" width="200px">';
print '<table bgcolor=#eeeeee valign=top cellpadding=2 cellspacing=2>';
print '<tr class="BlockHeaderLeftRight" style="height:18px"><td bgcolor="#1881b6" align=center style="font-family: Verdana; font-size: 11px; color:white">дата</td>';
print '<td bgcolor="#1881b6" align="center" style="font-family: Verdana; font-size: 11px; color:white">'.$sname.'</td></tr>';

$today=getdate();
if ($_GET["year"]=='') $ye=$today["year"];
else $ye=$_GET["year"];
if ($_GET["month"]=='') $mn=$today["mon"];
else $mn=$_GET["month"];
if ($_GET["day"]=='') $day=$today["mday"];
else $day=$_GET["day"];
//echo 'mn='.$mn;

$qnt2=89; $dy=31;
if (!checkdate ($mn,31,$ye)) { $dy=30; }
if (!checkdate ($mn,30,$ye)) { $dy=29; }
if (!checkdate ($mn,29,$ye)) { $dy=28; }

if ($_GET["month"]=='') if ($_GET["mday"]>1)$tm=$dy=$today["mday"]-1;
else $tm=$dy=$today["mday"]=$dy;

for ($tn=0; $tn<=$qnt2; $tn++)
    {
     $dat[$tn]=sprintf ("%d-%02d-%02d 00:00:00",$ye,$mn,$tm);
     $date2[$tn]=sprintf ("%02d-%02d",$mn,$tm);
//echo $date2[$tn];
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

 $query = 'SELECT * FROM prdata WHERE type=2 AND channel='.$_GET["id"].' ORDER BY date DESC';
// echo $query;
 if ($a = mysql_query ($query,$i))
 while ($uy = mysql_fetch_row ($a))
	{
//echo $uy[4];
	 for ($tn=1; $tn<=$qnt2; $tn++)
	     if ($uy[4]==$dat[$tn]) $x=$tn;
         $data2[$x]=number_format($uy[5],3);
        }

 $cm=0;
 for ($tn=0; $tn<=$qnt2; $tn++)
 if ($data2[$tn])
	{
	 print '<tr><td align=center bgcolor="#1881b6"><font style="font-family: Verdana; font-size: 11px; color:white">'.$dat[$tn].'</font></td>';
	 print '<td align=center bgcolor=#ffffff class="simple">'.$data2[$tn].'</td>';
	 print '</td></tr>';
	 $cm++;
	 if ($cm>10) break;
	}
print '</table></td>';

print '<td valign="top" width="200px">';
print '<table bgcolor=#eeeeee valign=top cellpadding=3 cellspacing=2>';
print '<tr class="BlockHeaderLeftRight" style="height:18px"><td bgcolor="#1881b6" align=center style="font-family: Verdana; font-size: 11px; color:white">дата</td>';
print '<td bgcolor="#1881b6" align="center" style="font-family: Verdana; font-size: 11px; color:white">'.$sname.'</td></tr>';

$today=getdate();
$ye=$today["year"];
$tm=$today["mon"];
$day=$today["mday"];

$qnt3=12; $cn=0;
for ($tn=0; $tn<=$qnt3; $tn++)
    {	 
     $dat[$cn]=sprintf ("%d-%02d-01 00:00:00",$ye,$tm);
     $dat2[$cn]=sprintf ("%d%02d01000000",$ye,$tm);
     $date3[$tn]=sprintf ("%02d",$mn);
//echo $dat2[$cn];
     if ($tm>1) $tm--;
     else { $tm=12; $ye--; }
     $cn++;
    }

$query = 'SELECT * FROM prdata WHERE type=4 AND channel='.$_GET["id"].' ORDER BY date DESC';
//echo $query;
if ($a = mysql_query ($query,$i))
while ($uy = mysql_fetch_row ($a))
	{
//echo $uy[4];
	 for ($tn=1; $tn<=$qnt3; $tn++)
	     if ($uy[4]==$dat[$tn]) $x=$tn;
         $data3[$x]=number_format($uy[5],3);
        }

 for ($tn=0; $tn<=$qnt3; $tn++)
 if ($data3[$tn])
	{
	 print '<tr><td align=center bgcolor="#1881b6"><font style="font-family: Verdana; font-size: 11px; color:white">'.$dat[$tn].'</font></td>';
	 print '<td align=center bgcolor=#ffffff class="simple">'.$data3[$tn].'</td>';
	 print '</td></tr>';
	}
print '</table></td>';

print '</tr></table>';

print '<table width=940 bgcolor=#eeeeee valign=top cellpadding=1 cellspacing=1>';
print '<tr><td class="simple">'; 
include("highcharts/trend.php"); 
print '</td></tr>';
print '<tr><td class="simple">'; 
include("highcharts/bar.php"); 
print '</td></tr>';
print '<tr><td class="simple" colspan=2></td></tr>';
print '</table></td></tr>';
print '</table>';
?>
</div></div></div>
</div>
</td></tr>
</tbody></table>