<script type="text/javascript" src="highcharts/jquery.min.js"></script>

<script type="text/javascript">
$(function () {
    var chart;
    $(document).ready(function() {
        chart = new Highcharts.Chart({
            chart: {
                renderTo: 'container',
                type: 'spline',
		marginRight: 10,
                events: {
                    load: function() {
                        // set up the updating of the chart each second
                        var series = this.series[0];
                        setInterval(function() {
                            var x = (new Date()).getTime(), // current time
                                y = Math.random();
                            series.addPoint([x, y], true, true);
                        }, 1000);
                    }
		},
            },
            title: {
                text: 'CPU Load (%)'
            },
	    xAxis: {
                type: 'datetime',
                tickPixelInterval: 150
            },
            yAxis: {
                title: {
                    text: null
                },
        	plotLines: [{
                    value: 0,
                    width: 1,
                    color: '#808080'
                }]
            },
            legend: {
                enabled: false
            },
            exporting: {
                enabled: false
            },
            series: [{
                name: 'CPU Load %',
                data: (function() {
                    var data = [], dat = [], time = (new Date()).getTime(), i;
            	    <?php
			include("config/local.php");
			$i = mysql_connect ($mysql_host,$mysql_user,$mysql_password); $e=mysql_select_db ($mysql_db_name);
			$query = 'SELECT * FROM escada.stat WHERE type=1 ORDER BY date DESC LIMIT 52';
			if ($e = mysql_query ($query,$i))
    			while ($ui = mysql_fetch_row ($e))
			    {
			     $str='dat.push('.number_format($ui[7],3).');';
			     print $str;
			    }
		    ?>
            	    for (i = -50; i <= 0; i++) {
                        data.push({
                            x: time + i * 1000,
                            y: dat[i+50]
                        });
                    }
                    return data;
                })()
	    }]
        });
    });
    
});
</script>

<script src="highcharts/js/highcharts.js"></script>
<script src="highcharts/js/modules/exporting.js"></script>
<div id="container" style="min-width: 300px; height: 250px; margin: 0 auto"></div>
