<?php
//TITLE=MySQL table patcher

$title = 'MySQL table patcher';
$progname = 'sqlupdate';

function usagetext($prog)
{
  exec($prog.' --help', $kk);
  $k='';foreach($kk as $s)$k.="$s\n";
  return $k;
}

$text = array(
   '1. Purpose' => "

 Reads a table creation file (tables.sql) and compares
 it to what mysqldump gives, and creates SQL clauses
 to update the database to match the creation file.
<pre class=smallerpre>".htmlspecialchars(usagetext('sqlupdate'))."</pre>

", '1. Changes' => "

 Since version 1.3.0, supports also the InnoDB <code>FOREIGN KEY</code> constraints.
<p>
 Since version 1.4.0, supports <code>TINYINT</code>, <code>DATE</code>, <code>TIME</code> and <code>ENUM</code> types.<br>
 Thanks to Leigh Purdie for this change!
<p>
 Version 1.4.1 adds support for <code>CREATE INDEX</code> and field ordering.
<p>
 Since version 1.5.0, supports <code>LONGTEXT</code>, <code>MEDIUMTEXT</code>
 and <code>MEDIUMBLOB</code>. Also now longer forces <code>DEFAULT</code> for
 <code>AUTOINCREMENT</code> columns.<br>
 Thanks to Frederic Lamsens for this change!
<p>
 In version 1.6.0, a crude support for <code>CHARSET</code> and <code>COLLATE</code>
 was added.
<p>
 In version 1.6.1, support for <code>GEOMETRY</code> and <code>SPATIAL</code>
 indexes was added.
<p>
 Version 1.6.2: Added -c option (ignore character set differences).
<p>
 In version 1.6.3, support for more spatial datatypes was added.
<p>
 In version 1.6.4, a bug in the support of <code>ENUM</code> was fixed.
 Thanks to Martin Vít for this change!
<p>
 In version 1.6.5, support was added for different index types in <code>PRIMARY KEY</code>.

", '1. Copying' => "

sqlupdate has been written by Joel Yliluoma, a.k.a.
<a href=\"http://iki.fi/bisqwit/\">Bisqwit</a>,<br>
and is distributed under the terms of the
<a href=\"http://www.gnu.org/licenses/licenses.html#GPL\">General Public License</a> (GPL).

", '1. Requirements' => "

GNU make is probably required.<br>
mysqldump is also required. This program handles only MySQL tables indeed.

");
include '/WWW/progdesc.php';
