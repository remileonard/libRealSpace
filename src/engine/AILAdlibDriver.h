//
//  AILAdlibDriver.h
//  libRealSpace
//
//  Partie "voix OPL" du pilote AdLib d'AIL 2.0 de Strike Commander (ADLIB.ADV), portee
//  ligne a ligne depuis le desassemblage (equivalent de YAMAHA.INC, absent des sources
//  publiques). Les noms sub_XXXX sont les routines du pilote (offset dans ADLIB.ADV).
//  Reference : strike_commander_re/analysis/ADLIB_DRIVER.md section 11.
//
//  16 voix logiques -> 9 canaux OPL2, allocation par priorite (sub_2514). Deux sortes de
//  timbres : OPL simple (longueur 0x0E, sub_2617) et TVFX a courbes (sub_89F / sub_552 /
//  sub_618, service a 60 Hz).
//
#pragma once
#include <cstdint>
#include <functional>
#include <vector>

class AILAdlibDriver {
public:
    static const int VOICES = 16;
    static const int OPL_CHANNELS = 9;
    static const int CACHE = 192;
    typedef std::function<void(uint16_t reg, uint8_t val)> WriteFn;

    // sub_1B47 (registres initiaux) + sub_1F4F (remise a zero des tables)
    void init(WriteFn write);
    // send_MIDI_message (sub_28BE)
    void send(int status, int d1, int d2);
    // serve_synth (sub_618) : a appeler a 120 Hz (TVFX a 60 Hz, priorites a 5 Hz)
    void serve();
    bool timbreStatus(int bank, int patch);
    // install_timbre (fonction AIL 0x9C) ; data commence par la longueur u16 du timbre
    void installTimbre(int bank, int patch, const uint8_t *data);
    int activeVoices() const;
    // OUTIL DE TEST, absent du pilote : coupe et libere toutes les voix
    void killAll();

    // --- etat du pilote (public pour l'affichage / le debogage) ---
    uint8_t c_bank[CACHE];   // 0x1422
    uint8_t c_patch[CACHE];  // 0x14E2
    uint8_t c_flags[CACHE];  // 0x15A2
    uint32_t c_stamp[CACHE]; // 0xFA2
    std::vector<uint8_t> c_data[CACHE];
    uint32_t stamp;
    uint8_t v_state[VOICES];         // 0x16D4 : 0 libre, 1 jouee, 2 relachement TVFX
    uint8_t v_type[VOICES];          // 0x16E4 : 0 OPL simple, 1/2 TVFX
    uint8_t v_opl[VOICES];           // 0x16F4 : canal OPL, 0xFF = aucun
    uint8_t v_chan[VOICES];          // 0x1704 : canal MIDI
    uint8_t v_note[VOICES];          // 0x1714
    uint8_t v_key[VOICES];           // 0x1724 : note MIDI recue (pour le Note Off)
    int8_t v_transpose[VOICES];      // 0x1734
    uint8_t v_vel[VOICES];           // 0x1744
    uint8_t v_sustained[VOICES];     // 0x1754
    uint8_t v_dirty[VOICES];         // 0x1764
    uint8_t v_b0[VOICES];            // 0x1774
    uint8_t v_keyon[VOICES];         // 0x1784
    uint8_t v_conn[VOICES];          // 0x1794
    uint8_t v_ksl0[VOICES];          // 0x17A4
    uint8_t v_ksl1[VOICES];          // 0x17B4
    uint8_t v_avekm0[VOICES];        // 0x17C4
    uint8_t v_avekm1[VOICES];        // 0x17D4
    uint8_t v_ad0[VOICES];           // 0x17E4
    uint8_t v_ad1[VOICES];           // 0x17F4
    uint8_t v_sr0[VOICES];           // 0x1804
    uint8_t v_sr1[VOICES];           // 0x1814
    uint8_t v_velmask[VOICES];       // 0x1824
    const uint8_t *v_timbre[VOICES]; // 0x1674
    uint16_t v_duration[VOICES];     // 0x16B4
    uint16_t v_prio_eff[VOICES];     // 0x197D
    // 8 parametres TVFX : curseur, compteur, valeur, increment
    uint16_t p_cur[8][VOICES];
    uint16_t p_cnt[8][VOICES];
    uint16_t p_val[8][VOICES];
    uint16_t p_inc[8][VOICES];
    uint8_t m_vol[16];
    uint8_t m_pan[16];
    uint8_t m_bend_l[16];
    uint8_t m_bend_h[16];
    uint8_t m_expr[16];
    uint8_t m_mod[16];
    uint8_t m_sustain[16];
    uint8_t m_vprot[16];
    uint8_t m_slot[16];
    uint8_t m_bank[16];
    uint8_t m_patch[16];
    uint8_t m_nvoices[16];
    uint8_t rhythm_slot[128];      // 0x18E4
    uint8_t o_owner[OPL_CHANNELS]; // 0x1974
    uint16_t rr_next;              // 0x166F
    uint16_t tick_tvfx;            // 0x166A
    uint16_t tick_prio;            // 0x166C
    uint8_t lvl_toggle;            // 0x166E

private:
    WriteFn writeFn;
    void wrOp(int op, int reg, uint8_t val);
    void wrCh(int ch, int reg, uint8_t val);
    int cacheFind(int bank, int patch);
    void updateVoice(int v);
    void releaseOpl(int v);
    void assign(int v, int opl);
    void reprioritize();
    void assignFree();
    void allocOpl(int v);
    void tvfxNext(int v, int p);
    void tvfxStart(int v);
    void bnkStart(int v);
    void noteOn(int ch, int key, int vel);
    void noteOff(int ch, int key);
    void releaseSustained(int ch);
    void applyDirty(int ch, uint8_t bits);
};
