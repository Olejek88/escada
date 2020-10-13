<script type="text/javascript" src="highcharts/jquery.min.js"></script>

<script type="text/javascript">
$(function () {
    var chart;
    $(document).ready(function() {
        chart = new Highcharts.Chart({
            chart: {
                renderTo: 'container',
                type: 'line'
            },
            xAxis: {
		minTickInterval: 5,
		tickInterval: 10,
                categories: [<?php
		  include("config/local.php");
		  $i = mysql_connect ($mysql_host,$mysql_user,$mysql_password); $e=mysql_select_db ($mysql_db_name);
		  $x=0;
		  $query = 'SELECT * FROM escada.stat WHERE type=1 ORDER BY date DESC LIMIT 50';
		  if ($e = mysql_query ($query,$i))
		  while ($ui = mysql_fetch_row ($e))
			{
		         $data1[$x]=$ui[1];
		         $dat1[$x]=$ui[7];
			 $x++;
			}
		  for ($dd=1;$dd<=50;$dd++)
			{
			 if ($dd>1) print ', ';
			 print '\''.substr($data1[50-$dd],12,5).'\'';
			}
		  print '],';
		?>
                labels: {
                    rotation: -45,
                    align: 'right',
                    style: {
                        fontSize: '10px',
                        fontFamily: 'Verdana, sans-serif'
                    }}
            },
            title: {
                text: 'CPU Load (%)'
            },
            yAxis: {
                title: {
                    text: null
                }
            },
            legend: {
                enabled: false
            },
            plotOptions: {
                line: {
                    dataLabels: {
                        enabled: false
                    },
                    enableMouseTracking: true
                }
            },
            series: [{
                name: 'CPU Load %',
		data: [<?php
		  for ($dd=1;$dd<=50;$dd++)
			{
			 if ($dd>1) print ', ';
			 print $dat1[50-$dd];
			}
		  print '] }]';
		?>
        });
    });
});
</script>

<script src="highcharts/js/highcharts.js"></script>
<script src="highcharts/js/modules/exporting.js"></script>
<div id="container" style="min-width: 300px; height: 250px; margin: 0 auto"></div>
