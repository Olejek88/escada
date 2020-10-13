<script type="text/javascript" src="jquery.min.js"></script>

<script type="text/javascript">
$(function () {
    var chart;
    $(document).ready(function() {
        chart = new Highcharts.Chart({
            chart: {
                renderTo: 'container',
                type: 'line'
            },
            title: {
		<?php
                 print 'text: \''.$_GET["title"].'\'';
		?>
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
                name: 'eqwewe',
		data: [<?php
		  include("../config/local.php");
		  $i = mysql_connect ($mysql_host,$mysql_user,$mysql_password); $e=mysql_select_db ($mysql_db_name);
		  $x=0;
		  $query = 'SELECT * FROM hours WHERE type=1 AND channel='.$_GET["chan"].' ORDER BY date DESC LIMIT 10';
		  if ($e = mysql_query ($query,$i))
		  while ($ui = mysql_fetch_row ($e))
			{
			 if ($x>0) print ', ';
			 print $ui[3]; 
			 $x++;			 
			}
		  print '] }]';
		?>
        });
    });
    
});
</script>

<script src="js/highcharts.js"></script>
<script src="js/modules/exporting.js"></script>
<div id="container" style="min-width: 800px; height: 250px; margin: 0 auto"></div>
