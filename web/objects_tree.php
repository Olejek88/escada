<script type="text/javascript" src="jstree/_lib/jquery.js"></script>
<script type="text/javascript" src="jstree/_lib/jquery.cookie.js"></script>
<script type="text/javascript" src="jstree/_lib/jquery.hotkeys.js"></script>
<script type="text/javascript" src="jstree/jquery.jstree.js"></script>

<link type="text/css" rel="stylesheet" href="jstree/_docs/syntax/!style.css"/>
<link type="text/css" rel="stylesheet" href="jstree/!style.css"/>
<script type="text/javascript" src="jstree/syntax/!script.js"></script>

<div id="demo2" class="demo"></div>
<script type="text/javascript" class="source">
$(function () {
    $("#demo2").jstree({
        "core" : { "initially_open" : [ "root" ] },
        "html_data" : {
            "data" : "<?php $query = 'SELECT * FROM objects WHERE type=1';
		 	    if ($a = mysql_query ($query,$i))
		  	    while ($uy = mysql_fetch_row ($a))
				{
				 print '<li id=\'root\'><a href=\'#\'>'.$uy[1].'</a><ul>';
				 $query = 'SELECT * FROM objects WHERE type=2 AND build='.$uy[0];
			 	 if ($a2 = mysql_query ($query,$i))
			  	 while ($uy2 = mysql_fetch_row ($a2))
					{
					 print '<li><a href=\'#\'>'.$uy2[1].'</a><ul>';
					 $query = 'SELECT * FROM objects WHERE type>2 AND build='.$uy[0].' AND nobj='.$uy2[0];
			 		 if ($a3 = mysql_query ($query,$i))
			  		 while ($uy3 = mysql_fetch_row ($a3))
					    {
					     print '<li><a href=\'#\'>'.$uy3[1].'</a></li>';
					    }
					 print '</ul></li>';
					}
				 print '</ul></li>"';
				}
			?>
        },
        "plugins" : [ "themes", "html_data" ]
    });
});
</script>