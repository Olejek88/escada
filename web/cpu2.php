<script type="text/javascript" src="highcharts/jquery.min.js"></script>

<script type="text/javascript">
$(function () {
    var chart;
    $(document).ready(function() {
        chart = new Highcharts.Chart({
            chart: {
                renderTo: 'container',
                type: 'line'
                events: {
                    load: function() {
    
                        // set up the updating of the chart each second
                        var series = this.series[0];
                        setInterval(function() {
                            var x = (new Date()).getTime(), // current time
                                y = Math.random();
                            series.addPoint([x, y], true, true);
                        }, 5000);
                    }
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
                        enabled: true
                    },
                    enableMouseTracking: false
                }
            },
            series: [{
                name: 'CPU Load %',
		data: [<?php
		  include("config/local.php");
		  $i = mysql_connect ($mysql_host,$mysql_user,$mysql_password); $e=mysql_select_db ($mysql_db_name);
		  $x=0;
		  $query = 'SELECT * FROM escada.stat WHERE type=1 ORDER BY date DESC LIMIT 60';
		  if ($e = mysql_query ($query,$i))
		  while ($ui = mysql_fetch_row ($e))
			{
			 if ($x>0) print ', ';
			 print $ui[7];
			 $x++;			 
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
