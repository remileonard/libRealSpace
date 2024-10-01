#include "RSHud.h"

void RSHud::parseREAL(uint8_t* data, size_t size) {
    IFFSaxLexer lexer;
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["CHUD"] = std::bind(&RSHud::parseREAL_CHUD, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSHud::parseREAL_CHUD(uint8_t* data, size_t size){
    IFFSaxLexer lexer;
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["LARG"] = std::bind(&RSHud::parseREAL_CHUD_LARG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SMAL"] = std::bind(&RSHud::parseREAL_CHUD_SMAL, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSHud::parseREAL_CHUD_LARG(uint8_t* data, size_t size){
    IFFSaxLexer lexer;
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["HINF"] = std::bind(&RSHud::parseREAL_CHUD_LARG_HINF, this, std::placeholders::_1, std::placeholders::_2);
    handlers["ASPD"] = std::bind(&RSHud::parseREAL_CHUD_LARG_ASPD, this, std::placeholders::_1, std::placeholders::_2);
    handlers["ALTI"] = std::bind(&RSHud::parseREAL_CHUD_LARG_ALTI, this, std::placeholders::_1, std::placeholders::_2);
    handlers["LADD"] = std::bind(&RSHud::parseREAL_CHUD_LARG_LADD, this, std::placeholders::_1, std::placeholders::_2);
    handlers["HEAD"] = std::bind(&RSHud::parseREAL_CHUD_LARG_HEAD, this, std::placeholders::_1, std::placeholders::_2);
    handlers["COLL"] = std::bind(&RSHud::parseREAL_CHUD_LARG_COLL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["STAL"] = std::bind(&RSHud::parseREAL_CHUD_LARG_STAL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["FUEL"] = std::bind(&RSHud::parseREAL_CHUD_LARG_FUEL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["LCOS"] = std::bind(&RSHud::parseREAL_CHUD_LARG_LCOS, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TARG"] = std::bind(&RSHud::parseREAL_CHUD_LARG_TARG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["MISD"] = std::bind(&RSHud::parseREAL_CHUD_LARG_MISD, this, std::placeholders::_1, std::placeholders::_2);
    handlers["CIRC"] = std::bind(&RSHud::parseREAL_CHUD_LARG_CIRC, this, std::placeholders::_1, std::placeholders::_2);
    handlers["CROS"] = std::bind(&RSHud::parseREAL_CHUD_LARG_CROS, this, std::placeholders::_1, std::placeholders::_2);
    handlers["CCIP"] = std::bind(&RSHud::parseREAL_CHUD_LARG_CCIP, this, std::placeholders::_1, std::placeholders::_2);
    handlers["CCRP"] = std::bind(&RSHud::parseREAL_CHUD_LARG_CCRP, this, std::placeholders::_1, std::placeholders::_2);
    handlers["STRF"] = std::bind(&RSHud::parseREAL_CHUD_LARG_STRF, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TTAG"] = std::bind(&RSHud::parseREAL_CHUD_LARG_TTAG, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSHud::parseREAL_CHUD_SMAL(uint8_t* data, size_t size){
    IFFSaxLexer lexer;
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["HINF"] = std::bind(&RSHud::parseREAL_CHUD_LARG_HINF, this, std::placeholders::_1, std::placeholders::_2);
    handlers["ASPD"] = std::bind(&RSHud::parseREAL_CHUD_LARG_ASPD, this, std::placeholders::_1, std::placeholders::_2);
    handlers["ALTI"] = std::bind(&RSHud::parseREAL_CHUD_LARG_ALTI, this, std::placeholders::_1, std::placeholders::_2);
    handlers["LADD"] = std::bind(&RSHud::parseREAL_CHUD_LARG_LADD, this, std::placeholders::_1, std::placeholders::_2);
    handlers["HEAD"] = std::bind(&RSHud::parseREAL_CHUD_LARG_HEAD, this, std::placeholders::_1, std::placeholders::_2);
    handlers["COLL"] = std::bind(&RSHud::parseREAL_CHUD_LARG_COLL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["STAL"] = std::bind(&RSHud::parseREAL_CHUD_LARG_STAL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["FUEL"] = std::bind(&RSHud::parseREAL_CHUD_LARG_FUEL, this, std::placeholders::_1, std::placeholders::_2);
    handlers["LCOS"] = std::bind(&RSHud::parseREAL_CHUD_LARG_LCOS, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TARG"] = std::bind(&RSHud::parseREAL_CHUD_LARG_TARG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["MISD"] = std::bind(&RSHud::parseREAL_CHUD_LARG_MISD, this, std::placeholders::_1, std::placeholders::_2);
    handlers["CIRC"] = std::bind(&RSHud::parseREAL_CHUD_LARG_CIRC, this, std::placeholders::_1, std::placeholders::_2);
    handlers["CROS"] = std::bind(&RSHud::parseREAL_CHUD_LARG_CROS, this, std::placeholders::_1, std::placeholders::_2);
    handlers["CCIP"] = std::bind(&RSHud::parseREAL_CHUD_LARG_CCIP, this, std::placeholders::_1, std::placeholders::_2);
    handlers["CCRP"] = std::bind(&RSHud::parseREAL_CHUD_LARG_CCRP, this, std::placeholders::_1, std::placeholders::_2);
    handlers["STRF"] = std::bind(&RSHud::parseREAL_CHUD_LARG_STRF, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TTAG"] = std::bind(&RSHud::parseREAL_CHUD_LARG_TTAG, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSHud::parseREAL_CHUD_LARG_HINF(uint8_t* data, size_t size){
    IFFSaxLexer lexer;
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&RSHud::parseREAL_CHUD_LARG_HINF_INFO, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSHud::parseREAL_CHUD_LARG_HINF_INFO(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_ASPD(uint8_t* data, size_t size){
    IFFSaxLexer lexer;
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&RSHud::parseREAL_CHUD_LARG_ASPD_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SHAP"] = std::bind(&RSHud::parseREAL_CHUD_LARG_ASPD_SHAP, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSHud::parseREAL_CHUD_LARG_ASPD_INFO(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_ASPD_SHAP(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_ALTI(uint8_t* data, size_t size){
    IFFSaxLexer lexer;
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&RSHud::parseREAL_CHUD_LARG_ALTI_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SHAP"] = std::bind(&RSHud::parseREAL_CHUD_LARG_ALTI_SHAP, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SHP2"] = std::bind(&RSHud::parseREAL_CHUD_LARG_ALTI_SHAP, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSHud::parseREAL_CHUD_LARG_ALTI_INFO(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_ALTI_SHAP(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_ALTI_SHP2(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_LADD(uint8_t* data, size_t size){
    IFFSaxLexer lexer;
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["VECT"] = std::bind(&RSHud::parseREAL_CHUD_LARG_LADD_VECT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["LADD"] = std::bind(&RSHud::parseREAL_CHUD_LARG_LADD_LADD, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSHud::parseREAL_CHUD_LARG_LADD_VECT(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_LADD_VECT_INFO(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_LADD_VECT_SHAP(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_LADD_LADD(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_LADD_LADD_INFO(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_HEAD(uint8_t* data, size_t size){
    IFFSaxLexer lexer;
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&RSHud::parseREAL_CHUD_LARG_HEAD_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SHAP"] = std::bind(&RSHud::parseREAL_CHUD_LARG_HEAD_SHAP, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SHP2"] = std::bind(&RSHud::parseREAL_CHUD_LARG_HEAD_SHP2, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSHud::parseREAL_CHUD_LARG_HEAD_INFO(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_HEAD_SHAP(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_HEAD_SHP2(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_COLL(uint8_t* data, size_t size){
    IFFSaxLexer lexer;
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&RSHud::parseREAL_CHUD_LARG_COLL_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SHAP"] = std::bind(&RSHud::parseREAL_CHUD_LARG_COLL_SHAP, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSHud::parseREAL_CHUD_LARG_COLL_INFO(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_COLL_SHAP(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_STAL(uint8_t* data, size_t size){
    IFFSaxLexer lexer;
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&RSHud::parseREAL_CHUD_LARG_STAL_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SHAP"] = std::bind(&RSHud::parseREAL_CHUD_LARG_STAL_SHAP, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSHud::parseREAL_CHUD_LARG_STAL_INFO(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_STAL_SHAP(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_FUEL(uint8_t* data, size_t size){
    IFFSaxLexer lexer;
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&RSHud::parseREAL_CHUD_LARG_FUEL_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SHAP"] = std::bind(&RSHud::parseREAL_CHUD_LARG_FUEL_SHAP, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSHud::parseREAL_CHUD_LARG_FUEL_INFO(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_FUEL_SHAP(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_LCOS(uint8_t* data, size_t size){
    IFFSaxLexer lexer;
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&RSHud::parseREAL_CHUD_LARG_LCOS_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SHAP"] = std::bind(&RSHud::parseREAL_CHUD_LARG_LCOS_SHAP, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSHud::parseREAL_CHUD_LARG_LCOS_INFO(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_LCOS_SHAP(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_TARG(uint8_t* data, size_t size){
    IFFSaxLexer lexer;
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&RSHud::parseREAL_CHUD_LARG_TARG_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SHAP"] = std::bind(&RSHud::parseREAL_CHUD_LARG_TARG_SHAP, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSHud::parseREAL_CHUD_LARG_TARG_INFO(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_TARG_SHAP(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_MISD(uint8_t* data, size_t size){
    IFFSaxLexer lexer;
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&RSHud::parseREAL_CHUD_LARG_MISD_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SHAP"] = std::bind(&RSHud::parseREAL_CHUD_LARG_MISD_SHAP, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSHud::parseREAL_CHUD_LARG_MISD_INFO(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_MISD_SHAP(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_CIRC(uint8_t* data, size_t size){
    IFFSaxLexer lexer;
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&RSHud::parseREAL_CHUD_LARG_CIRC_INFO, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSHud::parseREAL_CHUD_LARG_CIRC_INFO(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_CROS(uint8_t* data, size_t size){
    IFFSaxLexer lexer;
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&RSHud::parseREAL_CHUD_LARG_CROS_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SHAP"] = std::bind(&RSHud::parseREAL_CHUD_LARG_CROS_SHAP, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSHud::parseREAL_CHUD_LARG_CROS_INFO(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_CROS_SHAP(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_CCIP(uint8_t* data, size_t size){
    IFFSaxLexer lexer;
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&RSHud::parseREAL_CHUD_LARG_CCIP_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SHAP"] = std::bind(&RSHud::parseREAL_CHUD_LARG_CCIP_SHAP, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSHud::parseREAL_CHUD_LARG_CCIP_INFO(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_CCIP_SHAP(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_CCRP(uint8_t* data, size_t size){
    IFFSaxLexer lexer;
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&RSHud::parseREAL_CHUD_LARG_CCRP_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SHAP"] = std::bind(&RSHud::parseREAL_CHUD_LARG_CCRP_SHAP, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSHud::parseREAL_CHUD_LARG_CCRP_INFO(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_CCRP_SHAP(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_STRF(uint8_t* data, size_t size){
    IFFSaxLexer lexer;
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["INFO"] = std::bind(&RSHud::parseREAL_CHUD_LARG_STRF_INFO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SHAP"] = std::bind(&RSHud::parseREAL_CHUD_LARG_STRF_SHAP, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSHud::parseREAL_CHUD_LARG_STRF_INFO(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_STRF_SHAP(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_TTAG(uint8_t* data, size_t size){
    IFFSaxLexer lexer;
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["CLSR"] = std::bind(&RSHud::parseREAL_CHUD_LARG_TTAG_CLSR, this, std::placeholders::_1, std::placeholders::_2);
    handlers["TARG"] = std::bind(&RSHud::parseREAL_CHUD_LARG_TTAG_TARG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["NUMW"] = std::bind(&RSHud::parseREAL_CHUD_LARG_TTAG_NUMW, this, std::placeholders::_1, std::placeholders::_2);
    handlers["HUDM"] = std::bind(&RSHud::parseREAL_CHUD_LARG_TTAG_HUDM, this, std::placeholders::_1, std::placeholders::_2);
    handlers["IRNG"] = std::bind(&RSHud::parseREAL_CHUD_LARG_TTAG_IRNG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["GFRC"] = std::bind(&RSHud::parseREAL_CHUD_LARG_TTAG_GFRC, this, std::placeholders::_1, std::placeholders::_2);
    handlers["MAXG"] = std::bind(&RSHud::parseREAL_CHUD_LARG_TTAG_MAXG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["MACH"] = std::bind(&RSHud::parseREAL_CHUD_LARG_TTAG_MACH, this, std::placeholders::_1, std::placeholders::_2);
    handlers["WAYP"] = std::bind(&RSHud::parseREAL_CHUD_LARG_TTAG_WAYP, this, std::placeholders::_1, std::placeholders::_2);
    handlers["RALT"] = std::bind(&RSHud::parseREAL_CHUD_LARG_TTAG_RALT, this, std::placeholders::_1, std::placeholders::_2);
    handlers["LNDG"] = std::bind(&RSHud::parseREAL_CHUD_LARG_TTAG_LNDG, this, std::placeholders::_1, std::placeholders::_2);
    handlers["FLAP"] = std::bind(&RSHud::parseREAL_CHUD_LARG_TTAG_FLAP, this, std::placeholders::_1, std::placeholders::_2);
    handlers["SPDB"] = std::bind(&RSHud::parseREAL_CHUD_LARG_TTAG_SPDB, this, std::placeholders::_1, std::placeholders::_2);
    handlers["THRO"] = std::bind(&RSHud::parseREAL_CHUD_LARG_TTAG_THRO, this, std::placeholders::_1, std::placeholders::_2);
    handlers["CALA"] = std::bind(&RSHud::parseREAL_CHUD_LARG_TTAG_CALA, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}
void RSHud::parseREAL_CHUD_LARG_TTAG_CLSR(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_TTAG_TARG(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_TTAG_NUMW(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_TTAG_HUDM(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_TTAG_IRNG(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_TTAG_GFRC(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_TTAG_MAXG(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_TTAG_MACH(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_TTAG_WAYP(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_TTAG_RALT(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_TTAG_LNDG(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_TTAG_FLAP(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_TTAG_SPDB(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_TTAG_THRO(uint8_t* data, size_t size){
    
}
void RSHud::parseREAL_CHUD_LARG_TTAG_CALA(uint8_t* data, size_t size){
    
}
RSHud::RSHud() {

}
RSHud::~RSHud() {

}
void RSHud::InitFromRam(uint8_t* data, size_t size) {
    IFFSaxLexer lexer;
    std::map<std::string, std::function<void(uint8_t * data, size_t size)>> handlers;
    handlers["REAL"] = std::bind(&RSHud::parseREAL, this, std::placeholders::_1, std::placeholders::_2);
    lexer.InitFromRAM(data, size, handlers);
}