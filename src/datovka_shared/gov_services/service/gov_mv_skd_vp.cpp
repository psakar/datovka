/*
 * Copyright (C) 2014-2018 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations including
 * the two.
 */

#include "src/datovka_shared/gov_services/service/gov_mv_skd_vp.h"

static const char xml_template[] =
"<?xml version='1.0' encoding='utf-8'?>""\n"
"<d:root xmlns:d=\"http://software602.cz/sample\" ancestor_id=\"\" "
  "folder_id=\"\" formdata_id=\"\" fsuser_id=\"\" institute_type=\"\" "
  "ldapPass=\"\" nazev=\"\" page=\"0\" page_id=\"\" query_seq=\"1\" "
  "register=\"259\" retry=\"0\" seq=\"\" templateVersion_id=\"\" url=\"\" "
  "url_release=\"\" user_name=\"\" version=\"9.6\" xml:lang=\"cs\">""\n"
"  <d:gw_sn/>""\n"
"  <d:gw_form_name/>""\n"
"  <d:stav_form celkem_budov=\"\" celkem_jednotek=\"\" celkem_parcel=\"\" "
    "cp_cesta=\"\" cp_rejstrik=\"\" cp_test_cj=\"\" dalsi_strana_kn=\"0\" "
    "dalsi_strana_kn_c=\"\" dalsi_strana_orzr=\"0\" dalsi_strana_orzr_c=\"\" "
    "dolozka=\"\" dolozka_c=\"\" dolozka_cesta=\"\" dolozka_stitek=\"\" "
    "fuser_id=\"\" hash=\"\" hash_lv_sn=\"\" hash_sj_sn=\"\" hash_sp_sn=\"\" "
    "hashp=\"\" hashp_lv_sn=\"0\" hashp_sj_sn=\"\" hashp_sp2_tp=\"\" "
    "hashp_sp3_tp=\"\" hashp_sp4_tp=\"\" hashp_sp_sn=\"0\" hashp_sp_tp=\"0\" "
    "ir_datum_narozeni=\"\" ir_vyhledavani=\"0\" kd_vyhledavani=\"0\" "
    "kn_nemovitost=\"\" kn_typ_vypisu=\"\" kn_vyhledavani=\"0\" misto=\"\" "
    "misto_ini=\"\" or_typ_vypisu=\"\" p1_ir_ini=\"\" p1_ir_vypocet=\"\" "
    "p1_kd_ini=\"\" p1_kd_vypocet=\"\" p1_kn_ini=\"\" p1_kn_vypocet=\"\" "
    "p1_or_ini=\"\" p1_or_vypocet=\"\" p1_zr_ini=\"\" p1_zr_vypocet=\"\" "
    "p2_ir_ini=\"\" p2_ir_vypocet=\"\" p2_kd_ini=\"\" p2_kd_vypocet=\"\" "
    "p2_kn_ini=\"\" p2_kn_vypocet=\"\" p2_or_ini=\"\" p2_or_vypocet=\"\" "
    "p2_zr_ini=\"\" p2_zr_vypocet=\"\" p_ir=\"\" p_kd=\"\" p_kn=\"\" "
    "p_or=\"\" p_zr=\"\" pc_ini=\"\" pc_vypocet=\"\" platform=\"\" "
    "posta_zaver=\"0\" printer=\"\" printer_ChoosePrinter=\"\" "
    "printer_stitek=\"\" printer_test=\"\" prvni_strana_kn=\"0\" "
    "prvni_strana_kn_c=\"\" prvni_strana_orzr=\"0\" prvni_strana_orzr_c=\"\" "
    "savedialog=\"\" savedialog_test=\"\" saveform=\"\" saveform_posta=\"\" "
    "saveformdir=\"\" saveformdir_ini=\"\" sestavy_user_celkem=\"\" "
    "sestavy_user_nevydano=\"\" sestavy_user_vydano=\"\" "
    "seznam_parcel_id_nemovitost=\"\" sservice=\"\" sservice_ini=\"\" "
    "stav_cislo_jednotky=\"\" stav_dolozka=\"\" stav_dolozka_ok=\"\" "
    "stav_dolozka_stitek=\"\" stav_lv_obec=\"\" stav_lv_sb=\"\" "
    "stav_lv_seznam_nemovitosti=\"\" "
    "stav_lv_seznam_nemovitosti_priloha_pdf=\"\" "
    "stav_lv_sj=\"\" stav_lv_sp=\"\" stav_mmb=\"\" stav_nahled_pdf=\"\" "
    "stav_page=\"\" stav_pc_edit=\"0\" stav_pokladni_doklad=\"0\" "
    "stav_priloha_pdf=\"\" stav_printer_stitek=\"\" stav_retry=\"0\" "
    "stav_sestavy_user_nevydano=\"\" stav_seznam_nemovitosti=\"\" "
    "stav_seznam_nemovitosti_priloha_pdf=\"\" stav_sj_seznam_nemovitosti=\"\" "
    "stav_sj_seznam_nemovitosti_priloha_pdf=\"\" "
    "stav_sp_seznam_nemovitosti=\"\" "
    "stav_sp_seznam_nemovitosti_priloha_pdf=\"\" stav_testovani=\"\" "
    "stav_typ_parcela=\"0\" stav_zauctovane_vypisy=\"0\" stav_zkratka=\"\" "
    "transakcni_kod=\"\" typ_instituce=\"\" typform=\"\" url_fs=\"\" "
    "url_fs_ini=\"\" url_sservice=\"\" url_sservice_ini=\"\" urlir_fs=\"\" "
    "urlir_fs_ini=\"\" urlkd_fs=\"\" urlkd_fs_ini=\"\" urlkn_fs=\"\" "
    "urlkn_fs_ini=\"\" urlor_fs=\"\" urlor_fs_ini=\"\" urlzr_fs=\"\" "
    "urlzr_fs_ini=\"\" stav_hk_udaje=\"\"/>""\n"
"  <d:stav_rizeni>""\n"
"    <d:vydano>0</d:vydano>""\n"
"  </d:stav_rizeni>""\n"
"  <d:referent>""\n"
"    <d:prijmeni/>""\n"
"    <d:jmeno/>""\n"
"    <d:titpredjm/>""\n"
"    <d:titzajm/>""\n"
"    <d:datum_narozeni/>""\n"
"    <d:email/>""\n"
"    <d:telefon/>""\n"
"    <d:misto/>""\n"
"    <d:spisova_sluzba/>""\n"
"    <d:url_spisova_sluzba/>""\n"
"    <d:fs/>""\n"
"    <d:urlkn_fs/>""\n"
"    <d:urlor_fs/>""\n"
"    <d:urlzr_fs/>""\n"
"    <d:urlkd_fs/>""\n"
"    <d:url_fs/>""\n"
"    <d:cesta_formulare/>""\n"
"    <d:cislo_jednaci/>""\n"
"    <d:ucet_kn/>""\n"
"    <d:p1_kn>100</d:p1_kn>""\n"
"    <d:p2_kn>50</d:p2_kn>""\n"
"    <d:p1_or>100</d:p1_or>""\n"
"    <d:p2_or>50</d:p2_or>""\n"
"    <d:p1_zr>100</d:p1_zr>""\n"
"    <d:p2_zr>50</d:p2_zr>""\n"
"    <d:p1_kd>100</d:p1_kd>""\n"
"    <d:p2_kd>50</d:p2_kd>""\n"
"    <d:p1_ir>100</d:p1_ir>""\n"
"    <d:p2_ir>50</d:p2_ir>""\n"
"    <d:zkratka_uradu/>""\n"
"    <d:urlir_fs/>""\n"
"  </d:referent>""\n"
"  <d:vyhledavaci_udaje>""\n"
"    <d:ic typ_vypisu=\"1\"/>""\n"
"    <d:ku kod=\"\"/>""\n"
"    <d:lv/>""\n"
"    <d:ir>""\n"
"      <d:ic/>""\n"
"      <d:jmeno/>""\n"
"      <d:prijmeni/>""\n"
"      <d:datum_narozeni/>""\n"
"      <d:rc/>""\n"
"    </d:ir>""\n"
"    <d:kn err=\"\" nemovitost=\"\" seznam_nemovitosti=\"\" vyhledavani=\"0\">""\n"
"      <d:obec/>""\n"
"      <d:castecny_vypis>0</d:castecny_vypis>""\n"
"      <d:seznam_parcel err=\"\">""\n"
"        <d:ku/>""\n"
"        <d:kmenove_cislo/>""\n"
"        <d:poddeleni_cisla/>""\n"
"        <d:castecny_vypis>0</d:castecny_vypis>""\n"
"        <d:typ_parcely/>""\n"
"        <d:id_lv/>""\n"
"        <d:id/>""\n"
"        <d:nemovitost/>""\n"
"      </d:seznam_parcel>""\n"
"      <d:seznam_budov err=\"\" pocet_polozek=\"\" porovnat_polozku=\"\" "
        "vybrana_polozka=\"\">""\n"
"        <d:ku/>""\n"
"        <d:cast_obce/>""\n"
"        <d:typ_budovy>1</d:typ_budovy>""\n"
"        <d:cp_ce/>""\n"
"        <d:castecny_vypis>0</d:castecny_vypis>""\n"
"        <d:cast_obce_id/>""\n"
"      </d:seznam_budov>""\n"
"      <d:seznam_jednotek err=\"\">""\n"
"        <d:ku/>""\n"
"        <d:cast_obce/>""\n"
"        <d:typ_budovy>1</d:typ_budovy>""\n"
"        <d:cp_ce/>""\n"
"        <d:castecny_vypis>0</d:castecny_vypis>""\n"
"        <d:cislo_jednotky/>""\n"
"        <d:cast_obce_id/>""\n"
"        <d:nemovitost/>""\n"
"      </d:seznam_jednotek>""\n"
"      <d:opravneny_subjekt err=\"\">""\n"
"        <d:ku/>""\n"
"        <d:opravneny_subjekt/>""\n"
"        <d:opravneny_subjekt_id/>""\n"
"      </d:opravneny_subjekt>""\n"
"      <d:seznam_nemovitosti>""\n"
"        <d:seznam_parcel>""\n"
"          <d:polozka id=\"\" nemovitost=\"\"/>""\n"
"        </d:seznam_parcel>""\n"
"        <d:seznam_budov>""\n"
"          <d:polozka id=\"\" nemovitost=\"\"/>""\n"
"        </d:seznam_budov>""\n"
"        <d:seznam_jednotek>""\n"
"          <d:polozka id=\"\" id_lv=\"\" nemovitost=\"\"/>""\n"
"        </d:seznam_jednotek>""\n"
"      </d:seznam_nemovitosti>""\n"
"      <d:sestava/>""\n"
"    </d:kn>""\n"
"    <d:ciselnik>""\n"
"      <d:cast_obce>""\n"
"        <d:polozka cast_obce=\"\" id=\"\"/>""\n"
"      </d:cast_obce>""\n"
"      <d:opravneny_subjekt>""\n"
"        <d:polozka id=\"\" opravneny_subjekt=\"\"/>""\n"
"      </d:opravneny_subjekt>""\n"
"      <d:seznam_parcel>""\n"
"        <d:polozka id=\"\" id_lv=\"\" parcela=\"\"/>""\n"
"      </d:seznam_parcel>""\n"
"      <d:katastralni_uzemi>""\n"
"        <d:polozka id=\"\" katastralni_uzemi=\"\"/>""\n"
"      </d:katastralni_uzemi>""\n"
"      <d:seznam_nemovitosti seznam_nemovitosti=\"\">""\n"
"        <d:seznam_parcel>""\n"
"          <d:polozka id=\"\" id_nemovitost=\"\" nemovitost=\"\"/>""\n"
"        </d:seznam_parcel>""\n"
"        <d:seznam_budov>""\n"
"          <d:polozka id=\"\" nemovitost=\"\"/>""\n"
"        </d:seznam_budov>""\n"
"        <d:seznam_jednotek>""\n"
"          <d:polozka id=\"\" id_lv=\"\" nemovitost=\"\"/>""\n"
"        </d:seznam_jednotek>""\n"
"      </d:seznam_nemovitosti>""\n"
"      <d:sestavy>""\n"
"        <d:polozka datumVytvoreni=\"\" id=\"\" pocetStran=\"\" popis=\"\" "
          "stav=\"\"/>""\n"
"      </d:sestavy>""\n"
"    </d:ciselnik>""\n"
"    <d:kd>""\n"
"      <d:ic>__repl_IC_SUBJECT__</d:ic>""\n"
"      <d:icz/>""\n"
"    </d:kd>""\n"
"  </d:vyhledavaci_udaje>""\n"
"  <d:nahled_pdf page=\"0\" page_id=\"\">""\n"
"    <d:xml_data>""\n"
"      <d:seznam_parcel>""\n"
"        <d:data>""\n"
"          <d:par>""\n"
"            <d:id/>""\n"
"            <d:par_type/>""\n"
"            <d:katuze_kod/>""\n"
"            <d:kmenove_cislo_par/>""\n"
"            <d:poddeleni_cisla_par/>""\n"
"            <d:maplis_kod/>""\n"
"            <d:zpurvy_kod/>""\n"
"            <d:drupoz_kod/>""\n"
"            <d:vymera_parcely/>""\n"
"            <d:tel_id/>""\n"
"            <d:bud_id/>""\n"
"          </d:par>""\n"
"          <d:bud>""\n"
"            <d:id/>""\n"
"            <d:typbud_kod/>""\n"
"            <d:cislo_domovni/>""\n"
"          </d:bud>""\n"
"          <d:drupoz>""\n"
"            <d:kod/>""\n"
"            <d:nazev/>""\n"
"          </d:drupoz>""\n"
"          <d:zpurvy>""\n"
"            <d:kod/>""\n"
"            <d:nazev/>""\n"
"          </d:zpurvy>""\n"
"          <d:maplis>""\n"
"            <d:id/>""\n"
"            <d:oznaceni_mapoveho_listu/>""\n"
"          </d:maplis>""\n"
"          <d:katuze>""\n"
"            <d:kod/>""\n"
"            <d:obce_kod/>""\n"
"            <d:nazev/>""\n"
"          </d:katuze>""\n"
"          <d:opsub_rep>""\n"
"            <d:opsub>""\n"
"              <d:id/>""\n"
"              <d:opsub_type/>""\n"
"              <d:jmeno/>""\n"
"              <d:prijmeni/>""\n"
"              <d:nazev_ulice/>""\n"
"              <d:cislo_domovni/>""\n"
"              <d:obec/>""\n"
"              <d:psc/>""\n"
"              <d:nazev/>""\n"
"            </d:opsub>""\n"
"          </d:opsub_rep>""\n"
"          <d:vla>""\n"
"            <d:id/>""\n"
"            <d:opsub_id/>""\n"
"            <d:tel_id/>""\n"
"            <d:podil_citatel/>""\n"
"            <d:podil_jmenovatel/>""\n"
"          </d:vla>""\n"
"          <d:tel>""\n"
"            <d:id/>""\n"
"            <d:cislo_tel/>""\n"
"          </d:tel>""\n"
"        </d:data>""\n"
"      </d:seznam_parcel>""\n"
"      <d:seznam_budov>""\n"
"        <d:data>""\n"
"          <d:par>""\n"
"            <d:id/>""\n"
"            <d:par_type/>""\n"
"            <d:kmenove_cislo_par/>""\n"
"            <d:poddeleni_cisla_par/>""\n"
"          </d:par>""\n"
"          <d:bud>""\n"
"            <d:id/>""\n"
"            <d:typbud_kod/>""\n"
"            <d:caobce_kod/>""\n"
"            <d:cislo_domovni/>""\n"
"            <d:zpvybu_kod/>""\n"
"            <d:tel_id/>""\n"
"          </d:bud>""\n"
"          <d:typbud>""\n"
"            <d:kod/>""\n"
"            <d:nazev/>""\n"
"          </d:typbud>""\n"
"          <d:katuze>""\n"
"            <d:kod/>""\n"
"            <d:obce_kod/>""\n"
"            <d:nazev/>""\n"
"          </d:katuze>""\n"
"          <d:zpvybu>""\n"
"            <d:kod/>""\n"
"            <d:nazev/>""\n"
"          </d:zpvybu>""\n"
"          <d:opsub_rep>""\n"
"            <d:opsub>""\n"
"              <d:id/>""\n"
"              <d:opsub_type/>""\n"
"              <d:jmeno/>""\n"
"              <d:prijmeni/>""\n"
"              <d:nazev_ulice/>""\n"
"              <d:cislo_domovni/>""\n"
"              <d:obec/>""\n"
"              <d:psc/>""\n"
"              <d:nazev/>""\n"
"            </d:opsub>""\n"
"          </d:opsub_rep>""\n"
"          <d:vla>""\n"
"            <d:id/>""\n"
"            <d:opsub_id/>""\n"
"            <d:tel_id/>""\n"
"            <d:podil_citatel/>""\n"
"            <d:podil_jmenovatel/>""\n"
"          </d:vla>""\n"
"          <d:tel>""\n"
"            <d:id/>""\n"
"            <d:cislo_tel/>""\n"
"          </d:tel>""\n"
"        </d:data>""\n"
"      </d:seznam_budov>""\n"
"      <d:seznam_jednotek>""\n"
"        <d:data>""\n"
"          <d:bud>""\n"
"            <d:id/>""\n"
"            <d:typbud_kod/>""\n"
"            <d:cislo_domovni/>""\n"
"          </d:bud>""\n"
"          <d:katuze>""\n"
"            <d:kod/>""\n"
"            <d:obce_kod/>""\n"
"            <d:nazev/>""\n"
"          </d:katuze>""\n"
"          <d:jed>""\n"
"            <d:id/>""\n"
"            <d:bud_id/>""\n"
"            <d:typjed_kod/>""\n"
"            <d:cislo_jednotky/>""\n"
"            <d:zpvyje_kod/>""\n"
"            <d:tel_id/>""\n"
"            <d:podil_citatel/>""\n"
"            <d:podil_jmenovatel/>""\n"
"          </d:jed>""\n"
"          <d:typjed>""\n"
"            <d:kod/>""\n"
"            <d:nazev/>""\n"
"          </d:typjed>""\n"
"          <d:zpvyje>""\n"
"            <d:kod/>""\n"
"            <d:nazev/>""\n"
"          </d:zpvyje>""\n"
"          <d:opsub_rep>""\n"
"            <d:opsub>""\n"
"              <d:id/>""\n"
"              <d:opsub_type/>""\n"
"              <d:jmeno/>""\n"
"              <d:prijmeni/>""\n"
"              <d:nazev_ulice/>""\n"
"              <d:cislo_domovni/>""\n"
"              <d:obec/>""\n"
"              <d:psc/>""\n"
"              <d:nazev/>""\n"
"            </d:opsub>""\n"
"          </d:opsub_rep>""\n"
"          <d:vla>""\n"
"            <d:id/>""\n"
"            <d:opsub_id/>""\n"
"            <d:tel_id/>""\n"
"            <d:podil_citatel/>""\n"
"            <d:podil_jmenovatel/>""\n"
"          </d:vla>""\n"
"          <d:tel>""\n"
"            <d:id/>""\n"
"            <d:cislo_tel/>""\n"
"          </d:tel>""\n"
"        </d:data>""\n"
"      </d:seznam_jednotek>""\n"
"      <d:nahled_pdf_err/>""\n"
"    </d:xml_data>""\n"
"  </d:nahled_pdf>""\n"
"  <d:priloha_pdf stav=\"\">""\n"
"    <d:nazev_souboru/>""\n"
"    <d:pocet_stran ps_dolozka=\"\" tisk_dolozka=\"0\">0</d:pocet_stran>""\n"
"    <d:base64data/>""\n"
"  </d:priloha_pdf>""\n"
"  <d:evidencni_udaje filename=\"\" stav=\"\">""\n"
"    <d:pc gen=\"\" or=\"\" pc=\"\" pc_fs=\"\" pc_posta=\"\" stav=\"\"/>""\n"
"    <d:document_id stav=\"\"/>""\n"
"    <d:datum/>""\n"
"    <d:misto stav=\"\"/>""\n"
"    <d:typ_zadatele>1</d:typ_zadatele>""\n"
"    <d:fyzicka_osoba>""\n"
"      <d:prijmeni/>""\n"
"      <d:jmeno/>""\n"
"      <d:titpredjm/>""\n"
"      <d:titzajm/>""\n"
"      <d:datum_narozeni/>""\n"
"      <d:email/>""\n"
"      <d:telefon/>""\n"
"      <d:rc/>""\n"
"      <d:cp/>""\n"
"      <d:dp/>""\n"
"      <d:adresa>""\n"
"        <d:ulice/>""\n"
"        <d:cp_co/>""\n"
"        <d:psc/>""\n"
"        <d:obec/>""\n"
"      </d:adresa>""\n"
"      <d:adresa_bydliste>""\n"
"        <d:ulice/>""\n"
"        <d:cp_co/>""\n"
"        <d:psc/>""\n"
"        <d:obec/>""\n"
"      </d:adresa_bydliste>""\n"
"    </d:fyzicka_osoba>""\n"
"    <d:pravnicka_osoba>""\n"
"      <d:nazev_organizace/>""\n"
"      <d:ico/>""\n"
"      <d:prijmeni/>""\n"
"      <d:jmeno/>""\n"
"      <d:titpredjm/>""\n"
"      <d:titzajm/>""\n"
"      <d:email/>""\n"
"      <d:telefon/>""\n"
"      <d:rc/>""\n"
"      <d:cp/>""\n"
"      <d:datum_narozeni/>""\n"
"      <d:adresa>""\n"
"        <d:ulice/>""\n"
"        <d:cp_co/>""\n"
"        <d:psc/>""\n"
"        <d:obec/>""\n"
"      </d:adresa>""\n"
"      <d:adresa_bydliste>""\n"
"        <d:ulice1/>""\n"
"        <d:cp_co1/>""\n"
"        <d:psc1/>""\n"
"        <d:obec1/>""\n"
"      </d:adresa_bydliste>""\n"
"      <d:adresa_bydliste2>""\n"
"        <d:ulice2/>""\n"
"        <d:cp_co2/>""\n"
"        <d:psc02/>""\n"
"        <d:obec2/>""\n"
"      </d:adresa_bydliste2>""\n"
"    </d:pravnicka_osoba>""\n"
"  </d:evidencni_udaje>""\n"
"  <d:evidencni_udaje_cp>""\n"
"    <d:rejstrik/>""\n"
"    <d:pc/>""\n"
"  </d:evidencni_udaje_cp>""\n"
"  <d:prijmovy_doklad>""\n"
"    <d:prijmeni/>""\n"
"    <d:jmeno/>""\n"
"    <d:titpredjm/>""\n"
"    <d:titzajm/>""\n"
"    <d:datum_narozeni/>""\n"
"    <d:email/>""\n"
"    <d:telefon/>""\n"
"  </d:prijmovy_doklad>""\n"
"  <d:dolozka>""\n"
"    <d:registr/>""\n"
"    <d:pc/>""\n"
"    <d:pocet_stran/>""\n"
"    <d:misto/>""\n"
"    <d:datum/>""\n"
"    <d:referent_titpredjm/>""\n"
"    <d:referent_prijmeni/>""\n"
"    <d:referent_jmeno/>""\n"
"    <d:referent_titzajm/>""\n"
"  </d:dolozka>""\n"
"  <d:bindata>""\n"
"    <d:s_105x74/>""\n"
"    <d:s_105x74o/>""\n"
"    <d:s_90x36/>""\n"
"    <d:s_78x47/>""\n"
"    <d:nahledPDF/>""\n"
"  </d:bindata>""\n"
"  <d:evidencni_udaje_hk>""\n"
"    <d:nazev/>""\n"
"    <d:ulice_obec/>""\n"
"    <d:psc/>""\n"
"    <d:ic/>""\n"
"    <d:dic/>""\n"
"  </d:evidencni_udaje_hk>""\n"
"</d:root>";

#define IC_KEY "ic"

class MvSkdVpData {
	Q_DECLARE_TR_FUNCTIONS(MvSkdVpData)

public:
	MvSkdVpData(void)
	    : m_userIc()
	{ }

	QList<Gov::FormField> allFields(void) const;

private:
	QString m_userIc; /*!< Identification number. */
};

QList<Gov::FormField> MvSkdVpData::allFields(void) const
{
	QList<Gov::FormField> formList;

	{
		Gov::FormField ff;
		ff.setKey(IC_KEY);
		ff.setVal(m_userIc);
		ff.setDescr(tr("Subject ID (IČ)"));
		ff.setPlaceholder(tr("Enter subject ID (IČ)"));
		ff.setProperties(Gov::FormFieldType::PROP_MANDATORY |
		    Gov::FormFieldType::PROP_USER_INPUT);
		formList.append(ff);
	}

	return formList;
}

Gov::SrvcMvSkdVp::SrvcMvSkdVp(void)
    : Service()
{
	m_formFields = MvSkdVpData().allFields();
}

Gov::Service *Gov::SrvcMvSkdVp::createNew(void) const
{
	return new (std::nothrow) SrvcMvSkdVp;
}

const QString &Gov::SrvcMvSkdVp::internalId(void) const
{
	static const QString shortName("SrvcMvSkdVp");
	return shortName;
}

const QString &Gov::SrvcMvSkdVp::fullName(void) const
{
	static const QString fullName(tr("Printout from the list of qualified suppliers"));
	// "Výpis ze seznamu kvalifikovaných dodavatelů"
	return fullName;
}

const QString &Gov::SrvcMvSkdVp::instituteName(void) const
{
	static const QString instituteName(GOV_MV_FULL_NAME);
	return instituteName;
}

const QString &Gov::SrvcMvSkdVp::boxId(void) const
{
	static const QString boxId(GOV_MV_DB_ID);
	return boxId;
}

const QString &Gov::SrvcMvSkdVp::dmAnnotation(void) const
{
	static const QString dmAnnotation("CzechPOINT@home - Výpis ze seznamu kvalifikovaných dodavatelů");
	return dmAnnotation;
}

const QString &Gov::SrvcMvSkdVp::dmSenderIdent(void) const
{
	static const QString dmSenderIdent("CzechPOINT@home - 259");
	return dmSenderIdent;
}

const QString &Gov::SrvcMvSkdVp::dmFileDescr(void) const
{
	static const QString dmFileDescr(GOV_MV_XML_FILE_NAME);
	return dmFileDescr;
}

bool Gov::SrvcMvSkdVp::canSend(enum Isds::Type::DbType dbType) const
{
	switch (dbType) {
	case Isds::Type::BT_OVM_FO:
	case Isds::Type::BT_OVM_PFO:
	case Isds::Type::BT_OVM_PO:
	case Isds::Type::BT_PO:
	case Isds::Type::BT_PO_ZAK:
	case Isds::Type::BT_PO_REQ:
	case Isds::Type::BT_PFO:
	case Isds::Type::BT_PFO_ADVOK:
	case Isds::Type::BT_PFO_DANPOR:
	case Isds::Type::BT_PFO_INSSPR:
	case Isds::Type::BT_PFO_AUDITOR:
	case Isds::Type::BT_FO:
		return true;
		break;
	default:
		return false;
		break;
	}
}

bool Gov::SrvcMvSkdVp::setFieldVal(const QString &key, const QString &val)
{
	return Service::setFieldVal(key, val);
}

bool Gov::SrvcMvSkdVp::setOwnerInfoFields(const Isds::DbOwnerInfo &dbOwnerInfo)
{
	if (Q_UNLIKELY(dbOwnerInfo.isNull())) {
		return false;
	}

	return true;
}

bool Gov::SrvcMvSkdVp::haveAllValidFields(QString *errDescr)
{
	return Service::checkIc(IC_KEY, errDescr);
}

QByteArray Gov::SrvcMvSkdVp::binaryXmlContent(void) const
{
	QString xml(xml_template);
	xml.replace("__repl_IC_SUBJECT__", fieldVal(IC_KEY));
	return xml.toUtf8();
}
