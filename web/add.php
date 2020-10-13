<title>Webdata :: Настройка вычислителя</title>
<table cellpadding="0" cellspacing="1" border="0" style="width:1190px" align=center>
<?php
 if ($_GET["type"]=='build') print '<tr><td width=1190px bgcolor=#ffcf68 align=center><font class=tablz3>Ввод данных по объектам</font></td></tr>';
 if ($_GET["type"]=='devices') print '<tr><td width=1190px bgcolor=#ffcf68 align=center><font class=tablz3>Ввод данных по устройствам</font></td></tr>';
 if ($_GET["type"]=='objects') print '<tr><td width=1190px bgcolor=#ffcf68 align=center><font class=tablz3>Ввод данных по объектам</font></td></tr>';
 if ($_GET["type"]=='flats') print '<tr><td width=1190px bgcolor=#ffcf68 align=center><font class=tablz3>Ввод данных по квартирам</font></td></tr>';
 if ($_GET["type"]=='fields') print '<tr><td width=1190px bgcolor=#ffcf68 align=center><font class=tablz3>Настройка вычислений</font></td></tr>';

 if ($_GET["ud"]=='1')
	{
	 if ($_GET["type"]=='build') $query = 'DELETE FROM build WHERE id='.$_GET["id"];
	 if ($_GET["type"]=='objects') $query = 'DELETE FROM objects WHERE id='.$_GET["id"];
         if ($_GET["type"]=='flats') $query = 'DELETE FROM flats WHERE id='.$_GET["id"];
	 if ($_GET["type"]=='fields') $query = 'DELETE FROM field WHERE id='.$_GET["id"];
         if ($_GET["type"]=='devices') $query = 'DELETE FROM device WHERE idd='.$_GET["id"];
	 //echo $query.'<br>';
	 $a = mysql_query ($query,$i);
        }
 if ($_POST["frm"]=='1')
	{
	 if ($_POST["nid"]=='1')
		{ 
		 $query = 'SELECT startid FROM devicetype WHERE ids='.$_POST["n2"];
		 if ($a = mysql_query ($query,$i))
		 if ($uy = mysql_fetch_row ($a)) $startidd=$uy[0];

		 $query = 'SELECT MAX(idd) FROM device WHERE type='.$_POST["n2"];
		 if ($a = mysql_query ($query,$i))
		 if ($uy = mysql_fetch_row ($a)) $idd=$uy[0];  		  
		 if ($idd<100) $idd=$startidd+1;

                 if ($_POST["n5"]!='') $idd=$_POST["n5"];
		 if ($_POST["n9"]>0) $adr=$_POST["n9"]; else $adr=0x100;	

		 if ($_POST["n1"]=='' || $_POST["n1"]=='1') $_POST["n1"]=1;
		 if ($_POST["n1"]>='1')
			{
			 for ($dd=1; $dd<=$_POST["n1"]; $dd++)
				{
			 	 if ($_POST["n2"]==1) { $name='BIT (flat='.$_POST["n10"].',str='.$_POST["n11"].')'; }
			 	 if ($_POST["n2"]==2) { $name='2IP (flat='.$_POST["n10"].')'; }
			 	 if ($_POST["n2"]==4) { $name='MEE (flat='.$_POST["n10"].')'; }
			 	 if ($_POST["n2"]==5) { $name='IRP (strut='.$_POST["n11"].')'; }
			 	 if ($_POST["n2"]==6) { $name='LK (ent='.$_POST["n11"].',str='.$_POST["n11"].')'; }
			 	 if ($_POST["n2"]==11) $name='Tekon';
				 if ($_POST["n4"]!='') $name=$_POST["n4"];

				 $query = 'INSERT INTO device SET name=\''.$name.'\',type=\''.$_POST["n2"].'\',interface=\''.$_POST["n3"].'\',
						  idd=\''.$idd.'\',protocol=\''.$_POST["n6"].'\',port=\''.$_POST["n7"].'\',
						  speed=\''.$_POST["n8"].'\',adr=\''.$adr.'\',flat=\''.$_POST["n10"].'\'';
				 echo $query.'<br>';
				 $a = mysql_query ($query,$i);
                                 if ($_POST["n2"]==1)
					{
					 $query = 'INSERT INTO dev_bit SET device=\''.$idd.'\',rf_int_interval=\'3600\',ids_lk=\''.$_POST["n11"].'\',
						  ids_module=\''.$adr.'\',meas_interval=\'60\',integ_meas_cnt=\'60\',
						  flat_number=\''.$_POST["n10"].'\',strut_number=\''.$_POST["n12"].'\'';
					 echo $query.'<br>';
					 $a = mysql_query ($query,$i);
					}
				 $idd++; $adr++;
				}
			}
		}
	 if ($_POST["nid"]=='2')
		{
		 $query = 'INSERT INTO build SET name=\''.$_POST["n1"].'\',build_id=\''.$_POST["n2"].'\',nflats=\''.$_POST["n3"].'\',nentr=\''.$_POST["n4"].'\',descr=\''.$_POST["n5"].'\',nlevels=\''.$_POST["n6"].'\',nraion=\''.$_POST["n7"].'\'';
		 $a = mysql_query ($query,$i);					
		}
	 if ($_POST["nid"]=='3')
		{
		 $n9='Объект/Подъезд';
		 $query = 'INSERT INTO objects SET name=\''.$_POST["n1"].'\', type=\''.$_POST["n2"].'\', build=\'1\',strut=\''.$_POST["n4"].'\',block=\''.$_POST["n5"].'\',
						level=\''.$_POST["n6"].'\',flat=\''.$_POST["n7"].'\', nentr=\''.$_POST["n8"].'\',fname=\''.$n9.'\'';
		 $a = mysql_query ($query,$i);					
		}
	 if ($_POST["nid"]=='4')
		{
		 $query = 'INSERT INTO flats SET flat=\''.$_POST["n1"].'\',fname=\''.$_POST["n2"].'\',level=\''.$_POST["n3"].'\',rooms=\''.$_POST["n4"].'\',nstrut=\''.$_POST["n5"].'\',square=\''.$_POST["n6"].'\',ent=\''.$_POST["n7"].'\',rnum=\''.$_POST["n8"].'\',rasp=\''.$_POST["n9"].'\',unikod=\''.$_POST["n10"].'\',clic_schet=\''.$_POST["n11"].'\'';
		 $a = mysql_query ($query,$i);					
		}
	 if ($_POST["nid"]=='5')
		{
 		 if ($_POST["n3"]>'0' && $_POST["n4"]>'0')
			{				 			 
			 $query = 'INSERT INTO field SET type=1,mnem=\''.$_POST["n1"].'\',id1=\''.$_POST["n5"].'\',id2=\''.$_POST["n6"].'\',flat=\''.$_POST["n3"].'\',strut=\''.$_POST["n4"].'\',pip=\''.$_POST["n2"].'\'';
			 //echo $query.'<br>';
			 $a = mysql_query ($query,$i);					
			}
		}
	}
?>
<tr><td style="width:1190px" valign=top>
<table cellpadding="0" cellspacing="1" border="0" style="width:1190px" align=center>
<tr><td style="width:1190px" valign=top>
<table width=1190px cellpadding=2 cellspacing=1 bgcolor=#664466 align=center>
	<?php  
	  if ($_GET["type"]=='build') print '<tr><td bgcolor=#ffcf68 align=center width=60px><font class=tablz>id</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Объект</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Идентификатор</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Квартир</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Подъездов</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Описание</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Этажей</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Район</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Уд</font></td></tr>';
	  if ($_GET["type"]=='objects') print '<tr><td bgcolor=#ffcf68 align=center width=60px><font class=tablz>id</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Объект</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Тип</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Здание</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Стояк</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Блок</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Этаж</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Квартира</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Идентификатор</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Привязка</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Подъезд</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Полное имя</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Уд</font></td></tr>';
	  if ($_GET["type"]=='flats') print '<tr><td bgcolor=#ffcf68 align=center width=60px><font class=tablz>id</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Квартира</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Абонент</font></td>						 
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Этаж</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Комнат</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Стояков</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Площадь</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Подъезд</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Жильцов</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Расположение</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Код абонента</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Лицевой счет</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Уд</font></td></tr>';
	  if ($_GET["type"]=='fields') print '<tr><td bgcolor=#ffcf68 align=center width=60px><font class=tablz>id</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Мнемосхема</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Датчик 1</font></td>						 
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Датчик 2</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Квартира</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Расположение</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Стояк</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Уд</font></td></tr>';
	  if ($_GET["type"]=='devices') print '<tr><td bgcolor=#ffcf68 align=center><font class=tablz>Идентификатор</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Название</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Тип</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>SV</font></td>						 
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Интерфейс</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Протокол</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Порт</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Скорость</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Адрес</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Квартира</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Обмен</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>ЛК ID</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Модуль</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Стояк</font></td>
						 <td bgcolor=#ffcf68 align=center><font class=tablz>Уд</font></td></tr>';

	 include("config/local.php");
	 $i = mysql_connect ($mysql_host,$mysql_user,$mysql_password); $e=mysql_select_db ($mysql_db_name);
	 if ($_GET["type"]=='build') { $query = 'SELECT * FROM build'; $max=8; }
	 if ($_GET["type"]=='objects') { $query = 'SELECT  FROM objects';  $max=14; }
	 if ($_GET["type"]=='devices') { $query = 'SELECT idd,name,type,SV,interface,protocol,port,speed,adr,flat,lastdate FROM device ORDER BY type';  $max=10; }
	 if ($_GET["type"]=='flats') { $query = 'SELECT id,flat,fname,level,rooms,nstrut,square,ent,rnum,rasp,unikod,clic_schet FROM flats';  $max=12; }
	 if ($_GET["type"]=='fields') { $query = 'SELECT id,mnem,id1,id2,flat,pip,strut FROM field WHERE type=1';  $max=7; }
	 if ($a = mysql_query ($query,$i))
	 while ($uy = mysql_fetch_row ($a))
	      {          
	         print '<tr><td align=center bgcolor=#ffcf68 width=40><font class=top2>'.$uy[0].'</font></td>';
		 if ($_GET["type"]=='fields')
			{
			 print '<td align=center bgcolor=#e8e8e8><font class=top2>'.$uy[1].'</font></td>';

			  $query = 'SELECT name FROM device WHERE idd='.$uy[2];
		 	  if ($a2 = mysql_query ($query,$i))
			  if ($uy2 = mysql_fetch_row ($a2))			
				 print '<td align=center bgcolor=#e8e8e8><font class=top2>'.$uy2[0].' ['.$uy[2].']</font></td>';
			  else
				 print '<td align=center bgcolor=#e8e8e8><font class=top2>unknown ['.$uy[2].']</font></td>';

			  $query = 'SELECT name FROM device WHERE idd='.$uy[3];
		 	  if ($a2 = mysql_query ($query,$i))
			  if ($uy2 = mysql_fetch_row ($a2))			
				 print '<td align=center bgcolor=#e8e8e8><font class=top2>'.$uy2[0].' ['.$uy[3].']</font></td>';
			  else
				 print '<td align=center bgcolor=#e8e8e8><font class=top2>unknown ['.$uy[3].']</font></td>';

			 print '<td align=center bgcolor=#e8e8e8><font class=top2>'.$uy[4].'</font></td>';
			 if ($uy[5]==0) print '<td align=center bgcolor=#e8e8e8><font class=top2>прямой</font></td>';
			 else print '<td align=center bgcolor=#e8e8e8><font class=top2>обратной</font></td>';
			 print '<td align=center bgcolor=#e8e8e8><font class=top2>'.$uy[6].'</font></td>';
			}
		 else if ($_GET["type"]=='devices') 
			{ 
			 print '<td align=left bgcolor=#e8e8e8><font class=top2>'.$uy[1].'</font></td>';
			 if ($uy[2]==1) print '<td align=center bgcolor=#e8e8e8><font class=top2>БИТ</font></td>';
			 else if ($uy[2]==2) print '<td align=center bgcolor=#e8e8e8><font class=top2>2ИП</font></td>';
			 else if ($uy[2]==4) print '<td align=center bgcolor=#e8e8e8><font class=top2>МЭЭ</font></td>';
			 else if ($uy[2]==5) print '<td align=center bgcolor=#e8e8e8><font class=top2>ИРП</font></td>';
			 else if ($uy[2]==6) print '<td align=center bgcolor=#e8e8e8><font class=top2>ЛК</font></td>';
			 else if ($uy[2]==7) print '<td align=center bgcolor=#e8e8e8><font class=top2>ДК</font></td>';
			 else if ($uy[2]==11) print '<td align=center bgcolor=#e8e8e8><font class=top2>Тэкон</font></td>';
			 else print '<td align=center bgcolor=#e8e8e8><font class=top2>-</font></td>';
			 print '<td align=center bgcolor=#e8e8e8><font class=top2>'.$uy[3].'</font></td>';
			 if ($uy[4]==3) print '<td align=center bgcolor=#e8e8e8><font class=top2>Wireless</font></td>';
			 else if ($uy[4]==1) print '<td align=center bgcolor=#e8e8e8><font class=top2>RS-232</font></td>';
			 else if ($uy[4]==2) print '<td align=center bgcolor=#e8e8e8><font class=top2>RS-485</font></td>';
			 else if ($uy[4]==4) print '<td align=center bgcolor=#e8e8e8><font class=top2>Ethernet</font></td>';
			 else print '<td align=center bgcolor=#e8e8e8><font class=top2>'.$uy[4].'</font></td>';
			 print '<td align=center bgcolor=#e8e8e8><font class=top2>'.$uy[5].'</font></td>';
			 print '<td align=center bgcolor=#e8e8e8><font class=top2>ttyS'.$uy[6].'</font></td>';
			 print '<td align=center bgcolor=#e8e8e8><font class=top2>'.$uy[7].'</font></td>';
			 print '<td align=center bgcolor=#e8e8e8><font class=top2>'.$uy[8].'</font></td>';
			 print '<td align=center bgcolor=#e8e8e8><font class=top2>'.$uy[9].'</font></td>';
			 print '<td align=center bgcolor=#e8e8e8><font class=top2>'.$uy[10].'</font></td>';
			
			 if ($uy[2]==1) $query = 'SELECT ids_lk,ids_module,strut_number FROM dev_bit WHERE device='.$uy[0];
			 if ($uy[2]==2) $query = 'SELECT ids_lk,ids_module,flat_number FROM dev_2ip WHERE device='.$uy[0];
			 if ($uy[2]==4) $query = 'SELECT ids_lk,adr,flat FROM dev_mee WHERE device='.$uy[0];
			 if ($uy[2]==5) $query = 'SELECT 0,adr,strut FROM dev_irp WHERE device='.$uy[0];
			 if ($uy[2]==6) $query = 'SELECT 0,adr,level FROM dev_lk WHERE device='.$uy[0];			 
			 if ($a2 = mysql_query ($query,$i)) $uy2 = mysql_fetch_row ($a2);
			 print '<td align=center bgcolor=#e8e8e8><font class=top2>'.$uy2[0].'</font></td>';
			 print '<td align=center bgcolor=#e8e8e8><font class=top2>'.$uy2[1].'</font></td>';
			 print '<td align=center bgcolor=#e8e8e8><font class=top2>'.$uy2[2].'</font></td>';			 			
			}
		 else	
		 for ($n=1;$n<$max;$n++)		
			 print '<td align=center bgcolor=#e8e8e8><font class=top2>'.$uy[$n].'</font></td>';
		 if ($_GET["type"]=='build') print '<td align=center bgcolor=#e8e8e8><a href="index.php?sel=add&ud=1&type=build&id='.$uy[0].'"><img border=0 src="files/icon_delete.gif"></a></td>';
		 if ($_GET["type"]=='objects') print '<td align=center bgcolor=#e8e8e8><a href="index.php?sel=add&ud=1&type=objects&id='.$uy[0].'"><img border=0 src="files/icon_delete.gif"></a></td>';
                 if ($_GET["type"]=='flats') print '<td align=center bgcolor=#e8e8e8><a href="index.php?sel=add&ud=1&type=flats&id='.$uy[0].'"><img border=0 src="files/icon_delete.gif"></a></td>';
		 if ($_GET["type"]=='fields') print '<td align=center bgcolor=#e8e8e8><a href="index.php?sel=add&ud=1&type=fields&id='.$uy[0].'"><img border=0 src="files/icon_delete.gif"></a></td>';
                 if ($_GET["type"]=='devices') print '<td align=center bgcolor=#e8e8e8><a href="index.php?sel=add&ud=1&type=devices&id='.$uy[0].'"><img border=0 src="files/icon_delete.gif"></a></td>';
		 print '</tr>';
		}
	?>
</table>
</td>
</tr>
</table>

<table cellpadding="0" cellspacing="1" border="0" style="width:1190px" align=center>
	<?php  
	  print '<form name="frm1" method="post" action="index.php?sel=add&type='.$_GET["type"].'">';
	  if ($_GET["type"]=='build') print '<tr><td bgcolor=#ffcf68 align=center><font class=tablz>Добавить объект</font></td></tr>';
	  if ($_GET["type"]=='objects') print '<tr><td bgcolor=#ffcf68 align=center><font class=tablz>Добавить объект</font></td></tr>';
	  if ($_GET["type"]=='flats') print '<tr><td bgcolor=#ffcf68 align=center><font class=tablz>Добавить квартиру</font></td></tr>';
	  if ($_GET["type"]=='fields') print '<tr><td bgcolor=#ffcf68 align=center><font class=tablz>Добавить расчет</font></td></tr>';
	  if ($_GET["type"]=='devices') print '<tr><td bgcolor=#ffcf68 align=center><font class=tablz>Добавить устройство</font></td></tr>';

	  print '<tr><td style="width:1190px" valign=top>
		 <table width=1190px cellpadding=2 cellspacing=1 align=center>';

	  if ($_GET["type"]=='devices') 
		{
		  print '<tr><td>Количество устройств</td><td><input name="n1" size=30 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Тип устройств</td><td>
			 <select class=log id="type" name="n2" style="height:18">';
		  $query = 'SELECT * FROM devicetype';
	 	  if ($a = mysql_query ($query,$i))
		  while ($uy = mysql_fetch_row ($a))
			 print '<option value="'.$uy[4].'">'.$uy[1];
		  print '</select>
			 </td></tr>';
		  print '<tr><td>Интерфейс</td><td>
			 <select class=log id="type" name="n3" style="height:18">';
		  $query = 'SELECT * FROM interfaces';
	 	  if ($a = mysql_query ($query,$i))
		  while ($uy = mysql_fetch_row ($a))
			 print '<option value="'.$uy[0].'">'.$uy[1];
		  print '</select>
			 </td></tr>';
		  print '<tr><td>Название (пусто автоматом)</td><td><input name="n4" size=30 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Идентификатор (пусто автоматом)</td><td><input name="n5" size=5 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Протокол</td><td><input name="n6" size=5 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Порт</td><td><input name="n7" size=5 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Скорость</td><td>
			 <select class=log id="type" name="n8" style="height:18">';
			 print '<option value="300">300';
			 print '<option value="600">600';
			 print '<option value="1200">1200';
			 print '<option value="2400">2400';
			 print '<option value="4800">4800';
			 print '<option value="9600">9600';
			 print '<option value="19200">19200';
			 print '<option value="38400">38400';
			 print '<option value="57600">57600';
			 print '<option value="115200">115200';
		  print '</select>
			 </td></tr>';
		  print '<tr><td>Адрес (пусто автоматом)</td><td><input name="n9" size=5 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Квартира (пусто автоматом)</td><td><input name="n10" size=5 class=log style="height:18px"></td></tr>';
		  print '<tr><td>ЛК ID</td><td>
			 <select class=log id="type" name="n11" style="height:18">';
		  $query = 'SELECT * FROM device WHERE type=6';
	 	  if ($a = mysql_query ($query,$i))
		  while ($uy = mysql_fetch_row ($a))
			 print '<option value="'.$uy[7].'">'.$uy[20];
		  print '</select>
			 </td></tr>';
		  print '<tr><td>Стояк</td><td><input name="n12" size=5 class=log style="height:18px"></td></tr>';
		  print '<tr><td></td><td><input name="nid" size=1 style="height:1;width:1;visibility:hidden" value="1"></td></tr>';
		}

	  if ($_GET["type"]=='build') 
		{
		  print '<tr><td>Название</td><td><input name="n1" size=30 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Идентификатор</td><td><input name="n2" size=5 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Квартир</td><td><input name="n3" size=5 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Подъездов</td><td><input name="n4" size=5 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Этажей</td><td><input name="n5" size=5 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Район</td><td><input name="n6" size=5 class=log style="height:18px"></td></tr>';
		  print '<tr><td></td><td><input name="nid" size=1 style="height:1;width:1;visibility:hidden" value="2"></td></tr>';
		}
	  if ($_GET["type"]=='objects') 
		{
		  print '<tr><td>Название</td><td><input name="n1" size=30 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Тип</td><td>
			 <select class=log id="type" name="n2" style="height:18">
			 <option value="0">Неизвестно
			 <option value="1">Объект
			 <option value="2">Подъезд/Вход
			 <option value="3">Этаж
			 <option value="4">Стояк
			 <option value="5">Блок
			 <option value="6">Квартира
			 </select>
			 </td></tr>';
		  print '<tr><td>Объект</td><td><input name="n3" size=5 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Стояк</td><td>
			 <select class=log id="type" name="n4" style="height:18">';
		  $query = 'SELECT * FROM objects WHERE type=4';
	 	  if ($a = mysql_query ($query,$i))
		  while ($uy = mysql_fetch_row ($a))
		      	{
			 print '<option value="'.$uy[4].'">'.$uy[11];
			}
		  print '</select>';
		  print '</td></tr>';
		  print '<tr><td>Стояк</td><td>
			 <select class=log id="type" name="n5" style="height:18">
			 <option value="0">Нет блока
			 <option value="1">Левый блок
			 <option value="2">Правый блок
		  	 </select>';
		  print '</td></tr>';
		  print '<tr><td>Этаж</td><td>
			 <select class=log id="type" name="n6" style="height:18">
			 <option value="0">Не относится';
		  $query = 'SELECT * FROM objects WHERE type=3';
	 	  if ($a = mysql_query ($query,$i))
		  while ($uy = mysql_fetch_row ($a))
		      	{
			 print '<option value="'.$uy[4].'">'.$uy[11];
			}
		  print '</select>';
		  print '</td></tr>';
		  print '<tr><td>Квартира</td><td>
			 <select class=log id="type" name="n7" style="height:18">
			 <option value="0">Не относится';
		  $query = 'SELECT * FROM objects WHERE type=6';
	 	  if ($a = mysql_query ($query,$i))
		  while ($uy = mysql_fetch_row ($a))
		      	{
			 print '<option value="'.$uy[4].'">'.$uy[11];
			}
		  print '</select>';
		  print '</td></tr>';
		  print '<tr><td>Подъезд</td><td>
			 <select class=log id="type" name="n8" style="height:18">
			 <option value="0">Не относится';
		  $query = 'SELECT * FROM objects WHERE type=2';
	 	  if ($a = mysql_query ($query,$i))
		  while ($uy = mysql_fetch_row ($a))
		      	{
			 print '<option value="'.$uy[4].'">'.$uy[11];
			}
		  print '</select>';
		  print '</td></tr>';
		  print '<tr><td></td><td><input name="nid" size=1 style="height:1;width:1;visibility:hidden" value="3"></td></tr>';
		}
	  if ($_GET["type"]=='flats') 
		{
		  print '<tr><td>Квартира</td><td><input name="n1" size=20 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Абонент</td><td><input name="n2" size=30 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Этаж</td><td><input name="n3" size=5 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Комнат</td><td><input name="n4" size=5 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Стояков</td><td><input name="n5" size=5 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Площадь</td><td><input name="n6" size=5 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Подъезд</td><td><input name="n7" size=5 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Проживающих</td><td><input name="n8" size=5 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Расположение</td><td><input name="n9" size=5 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Уникальный код</td><td><input name="n10" size=15 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Лицевой счет</td><td><input name="n11" size=15 class=log style="height:18px"></td></tr>';
		  print '<tr><td></td><td><input name="nid" size=1 style="height:1;width:1;visibility:hidden" value="4"></td></tr>';
		}
	  if ($_GET["type"]=='fields') 
		{
		  print '<tr><td>Мнемосхема</td><td><input name="n1" size=30 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Направление</td><td>
			 <select class=log id="type" name="n2" style="height:18">
			 <option value="0">Подача
			 <option value="1">Обратка
			 </select>
			 </td></tr>';
		  print '<tr><td>Квартира</td><td><input name="n3" size=5 class=log style="height:18px"></td></tr>';
		  print '<tr><td>Стояк</td><td><input name="n4" size=5 class=log style="height:18px"></td></tr>';

		  print '<tr><td>Датчик входа</td><td>
			 <select class=log id="type" name="n5" style="height:18">';
		  $query = 'SELECT * FROM device WHERE type=1 OR type=5 ORDER BY name';
	 	  if ($a = mysql_query ($query,$i))
		  while ($uy = mysql_fetch_row ($a))
		      	{
			 print '<option value="'.$uy[1].'">'.$uy[20];
			}
		  print '</select>';
		  print '</td></tr>';

		  print '<tr><td>Датчик выхода</td><td>
			 <select class=log id="type" name="n6" style="height:18">';
		  $query = 'SELECT * FROM device WHERE type=1 OR type=5 ORDER BY name';
	 	  if ($a = mysql_query ($query,$i))
		  while ($uy = mysql_fetch_row ($a))
		      	{
			 print '<option value="'.$uy[1].'">'.$uy[20];
			}
		  print '</select>';
		  print '</td></tr>';
		  print '<tr><td></td><td><input name="nid" size=1 style="height:1;width:1;visibility:hidden" value="5"></td></tr>';
		}
	  print '<tr><td><input alt="add" name="Добавить" align=left type="submit"></td><td><input name="frm" size=1 style="height:1;width:1;visibility:hidden" value="1"></td></tr>';	    
	  print '</form>';
	?>
</td>
</tr>
</table>
</td></tr></table>

