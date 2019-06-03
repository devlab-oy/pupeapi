<?php

$uploaded_file = $_FILES['userfile']['tmp_name'];

if (is_uploaded_file($uploaded_file) === false) {
  echo "Tiedostoa ei lähetetty!";
  exit(1);
}

if ($_FILES['userfile']['size'] == 0) {
  echo "Tiedosto oli tyhjä!";
  exit(1);
}

exec("/home/devlab/devlab-web/api/sqlupdate/sqlupdate -t {$uploaded_file} -h localhost -u referenssi -p salainen_salasana_qwerty -d referenssi -r -c", $ulos);

$kielletyt = array(
  "CREATE TABLE ake_log",
  "CREATE TABLE autodata",
  "CREATE TABLE autodata_links",
  "CREATE TABLE autodata_oil",
  "CREATE TABLE autodata_repair_incjobs",
  "CREATE TABLE autodata_repair_text",
  "CREATE TABLE autodata_repair_times",
  "CREATE TABLE autodata_service_guide",
  "CREATE TABLE autodata_service_intervals",
  "CREATE TABLE autodata_service_notes",
  "CREATE TABLE autodata_service_operations",
  "CREATE TABLE autodata_service_strings",
  "CREATE TABLE autodata_tuote",
  "CREATE TABLE automanual_hakuhistoria",
  "CREATE TABLE luettelo_tilaukset",
  "CREATE TABLE rekisteritiedot",
  "CREATE TABLE sarjanumeron_lisatiedot",
  "CREATE TABLE tuotteen_orginaalit",
  "CREATE TABLE vertailu",
  "CREATE TABLE vertailu_hinnat",
  "CREATE TABLE vertailu_korvaavat",
  "CREATE TABLE yhteensopivuus_auto",
  "CREATE TABLE yhteensopivuus_autodata",
  "CREATE TABLE yhteensopivuus_rekisteri",
  "CREATE TABLE yhteensopivuus_tuote",
  "CREATE TABLE yhteensopivuus_tuote_lisatiedot",
  "CREATE TABLE yhteensopivuus_tuote_sensori",
  "CREATE TABLE auto_vari",
  "CREATE TABLE auto_vari_tuote",
  "CREATE TABLE auto_vari_korvaavat",
);

$ohitetut = array(
  "DROP TABLE autodata_link;",
  "DROP TABLE autodata_rt_incjob;",
  "DROP TABLE autodata_rt_text;",
  "DROP TABLE autodata_rt_time;",
  "DROP TABLE autodata_sg_guide;",
  "DROP TABLE autodata_sg_interval;",
  "DROP TABLE autodata_sg_note;",
  "DROP TABLE autodata_sg_operation;",
  "DROP TABLE autodata_sg_string;",
  "DROP TABLE autodata_td;",
  "DROP TABLE autodata_td_string;",
  "DROP TABLE autodata_td_strings;",
  "DROP TABLE df_cv_susp;",
  "DROP TABLE huolto;",
  "DROP TABLE huolto_asiakas;",
  "DROP TABLE huolto_asiakas_omarivi;",
  "DROP TABLE huolto_auto;",
  "DROP TABLE huolto_rivi;",
  "DROP TABLE huolto_rivi_tuote;",
  "DROP TABLE rekisteritiedot_lisatiedot;",
  "DROP TABLE sarjanumeron_lisatiedot;",
  "DROP TABLE td_cv;",
  "DROP TABLE td_cv_drivecab;",
  "DROP TABLE td_cv_drivecab_alloc;",
  "DROP TABLE td_cv_eng;",
  "DROP TABLE td_cv_susp;",
  "DROP TABLE td_eng;",
  "DROP TABLE td_manu;",
  "DROP TABLE td_model;",
  "DROP TABLE td_pc;",
  "DROP TABLE td_pc_add;",
  "DROP TABLE td_pc_eng;",
  "DROP TABLE yhteensopivuus_mp;",
  "DROP TABLE yhteensopivuus_valmistenumero;",
  "DROP TABLE futur_lasku;",
  "DROP TABLE futur_tiliointi;",
  "DROP TABLE futur_laskun_lisatiedot;",
  "DROP TABLE autoid_lisatieto;",
  "DROP TABLE futur_myyntper;",
  "DROP TABLE futur_myyntriv;",
  "DROP TABLE futur_tuotetyyppiriv;",
  "DROP TABLE futur_tuotper;",
  "DROP TABLE futur_tuotper2;",
  "DROP TABLE ajoneuvo_nakyvyys;",
  "DROP TABLE huoltosykli;",
  "DROP TABLE huoltosyklit_laitteet;",
  "DROP TABLE paikka;",
  "DROP TABLE yhteensopivuus_tuote_sensori_lisatiedot;",
  "DROP TABLE product_attachment;",
  "ALTER TABLE asiakas DROP herminator, DROP herminator1, DROP herminator2, DROP herminator3, DROP herminator4;",
  "ALTER TABLE tuote DROP tuotepituus, DROP vari, DROP suurin_henkiloluku, DROP laitetyyppi, DROP runkotyyppi, DROP kilpi, DROP materiaali, DROP koneistus, DROP sprinkleri, DROP teho_hv;",
  "ALTER TABLE maksuehto DROP herminator;",
  "ALTER TABLE toimi DROP herminator;",
  "ALTER TABLE toimitustapa DROP herminator, DROP herminator2;",
  "ALTER TABLE yhteyshenkilo DROP herminator;",
  "USE referenssi;",
  "ALTER TABLE tuotepaikat DROP inventointilista, DROP inventointilista_aika, DROP inventointilista_naytamaara, DROP INDEX yhtio_inventointilista_aika, DROP INDEX yhtio_inventointilista;",
  "SET FOREIGN_KEY_CHECKS=0;",
  "SET FOREIGN_KEY_CHECKS=1;",
);

$jarru = "OFF";

foreach ($ulos as $print) {

  $print = str_replace(" 0000-00-00 00:00:00,"," '0000-00-00 00:00:00',", $print);
  $print = str_replace(" 0000-00-00 00:00:00 "," '0000-00-00 00:00:00' ", $print);
  $print = str_replace(" 0000-00-00,"," '0000-00-00',", $print);
  $print = str_replace(" 0000-00-00 "," '0000-00-00' ", $print);
  $print = str_replace(" 0000-00-00;"," '0000-00-00';", $print);
  $print = str_replace(" 00:00:00,"," '00:00:00',", $print);
  $print = str_replace(" 00:00:00 "," '00:00:00' ", $print);

  $print = str_replace("abs_pvm DATE NULL DEFAULT '0000-00-00'",
                       "abs_pvm DATE NULL DEFAULT NULL", $print);

  $print = str_replace("kassa_abspvm DATE NULL DEFAULT '0000-00-00'",
                       "kassa_abspvm DATE NULL DEFAULT NULL", $print);

  if (in_array(trim($print), $kielletyt)) {
    $jarru = "ON";
  }

  if ($jarru == "OFF" and !in_array(trim($print), $ohitetut)) {
    echo "$print\n";
  }

  if ($jarru == "ON" and trim(strtoupper($print)) == ");") {
    $jarru = "OFF";
  }

}
