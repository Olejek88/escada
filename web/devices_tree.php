<script type="text/javascript" src="jstree/_lib/jquery.js"></script>
<script type="text/javascript" src="jstree/_lib/jquery.cookie.js"></script>
<script type="text/javascript" src="jstree/_lib/jquery.hotkeys.js"></script>
<script type="text/javascript" src="jstree/jquery.jstree.js"></script>

<link type="text/css" rel="stylesheet" href="jstree/_docs/syntax/!style.css"/>
<link type="text/css" rel="stylesheet" href="jstree/!style.css"/>
<script type="text/javascript" src="jstree/syntax/!script.js"></script>

<div id="demo2" class="demo" style="font-size:10px"></div>
<script type="text/javascript" class="source">
$(function () {
    $("#demo2").jstree({
        "core" : { "initially_open" : [ "root" ] },
        "html_data" : {
            "data" : "<?php $query = 'SELECT * FROM dev_dk';
		 	    if ($a = mysql_query ($query,$i))
		  	    while ($uy = mysql_fetch_row ($a))
				{
				 print '<li id=\'root\'><a href=\'#\'>'.$uy[14].'</a><ul>';
				 $query = 'SELECT interface FROM '.$device_table.' WHERE interface!=3 GROUP BY interface';
			 	 if ($a2 = mysql_query ($query,$i))
			  	 while ($uy2 = mysql_fetch_row ($a2))
					{		
					 $query = 'SELECT * FROM interfaces WHERE id='.$uy2[0];
				 	 if ($a3 = mysql_query ($query,$i))
				  	 if ($uy3 = mysql_fetch_row ($a3)) 		
						 print '<li><a href=\'#\'>'.$uy3[1].'</a><ul>';
					 else    print '<li><a href=\'#\'>unknown</a><ul>';

					 $query = 'SELECT port FROM '.$device_table.' WHERE interface='.$uy2[0].' GROUP BY port';
				 	 if ($a3 = mysql_query ($query,$i))
				  	 while ($uy3 = mysql_fetch_row ($a3))
						{
                                                 if ($uy2[0]==4) print '<li><a href=\'#\'>eth'.(255-$uy3[0]).'</a><ul>';
						 else print '<li><a href=\'#\'>dev/ttyS'.$uy3[0].'</a><ul>';
						 $query = 'SELECT * FROM '.$device_table.' WHERE port='.$uy3[0].' ORDER BY adr';
					 	 if ($a4 = mysql_query ($query,$i))
					  	 while ($uy4 = mysql_fetch_row ($a4))
				                        {
	                                                 if ($uy2[0]==4) print '<li><a href=\'#\'>'.$uy4[20].' ('.$uy4[9].')</a><ul>';
							 else print '<li><a href=\'#\'>'.$uy4[20].' (adr='.$uy4[7].')</a><ul>';
						 	 $query = 'SELECT * FROM channels WHERE device='.$uy4[1];
						 	 if ($a5 = mysql_query ($query,$i))
						  	 while ($uy5 = mysql_fetch_row ($a5))
					                        {
		                                                 print '<li><a href=\'#\'>'.$uy5[1].'</a></li>';
					                        }
					                 if ($uy4[8]==6) // LK
								{
							 	 $query = 'SELECT * FROM dev_bit WHERE ids_lk='.$uy4[7];
							 	 if ($a5 = mysql_query ($query,$i))
							  	 while ($uy5 = mysql_fetch_row ($a5))
									{
			                                                 print '<li><a href=\'#\'>ахр [0x'.$uy5[3].'|0x'.$uy5[4].']</a><ul>';
								 	 $query = 'SELECT * FROM channels WHERE device='.$uy5[1];
								 	 if ($a6 = mysql_query ($query,$i))
								  	 while ($uy6 = mysql_fetch_row ($a6))
					        		                {
		                                                		 print '<li><a href=\'index.php?sel=channel&id='.$uy6[0].'\'>'.$uy6[1].'</a></li>';
							                        }
									 print '</ul></li>';
									}
							 	 $query = 'SELECT * FROM dev_2ip WHERE ids_lk='.$uy4[7];
							 	 if ($a5 = mysql_query ($query,$i))
							  	 while ($uy5 = mysql_fetch_row ($a5))
			                                                 print '<li><a href=\'#\'>2хо [0x'.$uy5[3].'|0x'.$uy5[4].']</a></li>';
								}
						 	 print '</ul></li>';							 
							}
						 print '</ul></li>';
						}
					 print '</ul></li>';
					}
				 print '</ul></li>"';
				}
			?>
        },
	"themes" : {
         "theme" : "apple",
         "dots" : true,
         "icons" : true },
        "plugins" : [ "themes", "html_data" ]
    });
});
</script>