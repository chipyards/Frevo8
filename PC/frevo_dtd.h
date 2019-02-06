
/* des DTD (Document Type Definition)
   derivees de la classe de base DTD de xmlpb.h,
   elles ne different que par le constructeur, specifique de l'application
 */

class DTD_four : public DTD {
public :
DTD_four() {
  DTDelem e;
  e.attrib["version"] = 1;
  e.attrib["xmldir"] = 1;
  e.attrib["pixdir"] = 1;
  e.attrib["plotdir"] = 1;
  elem["frevo"] = e;
  e.attrib.clear();

  e.attrib["n"] = 1;
  e.attrib["title"] = 1;
  e.attrib["ip"] = 1;
  e.attrib["locdir"] = 1;
  e.attrib["comm_verbose"] = 1;
  e.attrib["comm_log"] = 1;
  e.attrib["auto_secu"] = 1;
  elem["four"] = e;
  e.attrib.clear();

  e.attrib["n"] = 1;
  e.attrib["txt"] = 1;
  e.attrib["src0"] = 1; e.attrib["src1"] = 1;
  e.attrib["x"] = 1;    e.attrib["y"] = 1;
  e.attrib["gazx"] = 1; e.attrib["gazy"] = 1;
  e.attrib["pixx"] = 1; e.attrib["pixy"] = 1;
  e.attrib["pix"] = 1;
  elem["vanne"] = e;
  e.attrib.clear();

  e.attrib["n"] = 1;
  e.attrib["txt"] = 1;
  e.attrib["src"] = 1;
  e.attrib["fs"] = 1;
  e.attrib["unit"] = 1;
  e.attrib["x"] = 1;    e.attrib["y"] = 1;
  e.attrib["pixx"] = 1; e.attrib["pixy"] = 1;
  e.attrib["gazx"] = 1; e.attrib["gazy"] = 1;
  elem["mfc"] = e;
  e.attrib.clear();

  e.attrib["n"] = 1;
  e.attrib["txt"] = 1;
  e.attrib["x"] = 1;    e.attrib["y"] = 1;
  e.attrib["pixx"] = 1; e.attrib["pixy"] = 1;
  e.attrib["temp0"] = 1;
  e.attrib["temp1"] = 1;
  e.attrib["temp2"] = 1;
  elem["tem"] = e;
  e.attrib.clear();

  e.attrib["txt"] = 1;
  e.attrib["x"] = 1;    e.attrib["y"] = 1;
  e.attrib["pixx"] = 1; e.attrib["pixy"] = 1;
  e.attrib["fs"] = 1;
  e.attrib["unit"] = 1;
  e.attrib["offset"] = 1;
  elem["fre"] = e;

  }; // fin constructeur
};

class DTD_recette : public DTD {
public :
DTD_recette() {
  DTDelem e;
  e.attrib["four"] = 1;
  e.attrib["titre"] = 1;
  elem["recette"] = e;
  e.attrib.clear();

  e.attrib["n"] = 1;
  e.attrib["titre"] = 1;
  e.attrib["duree"] = 1;
  e.attrib["deldg"] = 1;
  e.attrib["saut"] = 1;
  elem["step"] = e;
  e.attrib.clear();

  e.attrib["n"] = 1;
  elem["vanne"] = e;
  e.attrib.clear();

  e.attrib["n"] = 1;
  e.attrib["SV"] = 1;
  e.attrib["SVmi"] = 1;
  e.attrib["SVma"] = 1;
  e.attrib["SVinc"] = 1;
  e.attrib["SVdec"] = 1;
  e.attrib["check"] = 1;
  e.attrib["rampe"] = 1;
  e.attrib["saut"] = 1;
  elem["mfc"] = e;
  elem["tem"] = e;
  e.attrib["offset"] = 1;
  elem["fre"] = e;

  /*
  e.attrib[""] = 1;
  elem[""] = e;
  e.attrib.clear();
  */

  }; // fin constructeur
};
