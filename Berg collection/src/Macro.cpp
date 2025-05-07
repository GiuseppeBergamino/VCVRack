#include "plugin.hpp"

#define NUM_CONTROLLI 6

float mappaCV(float inputVal, float mapMin, float mapMax) {
    
    float in = rack::math::clamp(inputVal, -10.f, 10.f);
    float t = (in + 10.f) * 0.05f;        // = (in + 10) / 20
    return mapMin + t * (mapMax - mapMin);
}

struct Macro : Module {
	enum ParamId {
        MIN1_PARAM,
        MIN2_PARAM,
        MIN3_PARAM,
        MIN4_PARAM,
        MIN5_PARAM,
        MIN6_PARAM,
        MAX1_PARAM,
        MAX2_PARAM,
        MAX3_PARAM,
        MAX4_PARAM,
        MAX5_PARAM,
        MAX6_PARAM,
        MACRO_PARAM,
        PLAY_PARAM,
        REC_PARAM,
        ERASE_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		MACRO_IN,
        PLAY_IN,
        REC_IN,
		INPUTS_LEN
	};
	enum OutputId {
		CV1_OUT,
        CV2_OUT,
        CV3_OUT,
        CV4_OUT,
        CV5_OUT,
        CV6_OUT,
        GATE1_OUT,
        GATE2_OUT,
        GATE3_OUT,
        GATE4_OUT,
        GATE5_OUT,
        GATE6_OUT,
		OUTPUTS_LEN
	};
    enum LightsId {
        GATE1_LED,
        GATE2_LED,
        GATE3_LED,
        GATE4_LED,
        GATE5_LED,
        GATE6_LED,
        PLAY_LED,
        REC_LED,
        ERASE_LED,
        LIGHTS_LEN
    };
    
    float potMin[NUM_CONTROLLI] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
    float potMax[NUM_CONTROLLI] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
    float outCV[NUM_CONTROLLI]  = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
    bool  outGATE[NUM_CONTROLLI] = {0, 0, 0, 0, 0, 0};
    
    dsp::SchmittTrigger playDetector;
    dsp::SchmittTrigger playGateDetector;
    dsp::SchmittTrigger recDetector;
    dsp::SchmittTrigger recGateDetector;
    dsp::SchmittTrigger eraseDetector;
  
    const float stepRec = 0.015f; //secondi di campionamento per registrazione
    int sampleRec = 0;
    int samplePlay = 0;
    float bufferRec[512] = {}; // 7.68 secondi di registrazione
    int indiceRec = 0;
    int indicePlay = 0;
    int contaFrame = 0;
    bool rec = false;
    bool play = false;
    bool erase = false;
    float macroPlay = 0.f;
    dsp::PulseGenerator luceErase;

    std::string noteText;       // stringa permanente per memorizzare il testo
    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        //--buffer rec
        json_t* arrJ = json_array();
            for (int i = 0; i < indiceRec; ++i)
                json_array_append_new(arrJ, json_real(bufferRec[i]));
            json_object_set_new(rootJ, "recBuf", arrJ);
            json_object_set_new(rootJ, "recLen", json_integer(indiceRec));
        //--testo etichetta
        json_object_set_new(rootJ, "note", json_string(noteText.c_str()));
        return rootJ;
        
    }
    void dataFromJson(json_t* rootJ) override {
        //--ripristina buffer rec
        json_t* arrJ = json_object_get(rootJ, "recBuf");
        json_t* lenJ = json_object_get(rootJ, "recLen");
        if (arrJ && lenJ) {
            indiceRec = json_integer_value(lenJ);
            if (indiceRec > 512) indiceRec = 512;
            for (int i = 0; i < indiceRec; ++i)
                bufferRec[i] = (float)json_real_value(json_array_get(arrJ, i));
        }
        //--rirpistina label
        json_t* sJ = json_object_get(rootJ, "note");
        if (sJ)
            noteText = json_string_value(sJ);
    }


    Macro() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        sampleRec = int(APP->engine->getSampleRate() * stepRec + 0.5f);
        
		configParam(MACRO_PARAM, -10.f, 10.f, 0.f, "Macro", " V");
        configInput(MACRO_IN, "Macro CV");
        configInput(PLAY_IN, "Play/Stop");
        configInput(REC_IN, "Rec On/Off");
        
        configParam(MIN1_PARAM, -10.f, 10.f, -10.f, "Map min 1", " V");
        configParam(MAX1_PARAM, -10.f, 10.f, 10.f, "Map max 1", " V");
        configOutput(CV1_OUT, "CV 1");
        configOutput(GATE1_OUT, "Gate 1");
        configLight(GATE1_LED, "Gate LED");
        
        configParam(MIN2_PARAM, -10.f, 10.f, -10.f, "Map min 2", " V");
        configParam(MAX2_PARAM, -10.f, 10.f, 10.f, "Map max 2", " V");
        configOutput(CV2_OUT, "CV 2");
        configOutput(GATE2_OUT, "Gate 2");
        configLight(GATE2_LED, "Gate LED");
        
        configParam(MIN3_PARAM, -10.f, 10.f, -10.f, "Map min 3", " V");
        configParam(MAX3_PARAM, -10.f, 10.f, 10.f, "Map max 3", " V");
        configOutput(CV3_OUT, "CV 3");
        configOutput(GATE3_OUT, "Gate 3");
        configLight(GATE3_LED, "Gate LED");
        
        configParam(MIN4_PARAM, -10.f, 10.f, -10.f, "Map min 4", " V");
        configParam(MAX4_PARAM, -10.f, 10.f, 10.f, "Map max 4", " V");
        configOutput(CV4_OUT, "CV 4");
        configOutput(GATE4_OUT, "Gate 4");
        configLight(GATE4_LED, "Gate LED");
        
        configParam(MIN5_PARAM, -10.f, 10.f, -10.f, "Map min 5", " V");
        configParam(MAX5_PARAM, -10.f, 10.f, 10.f, "Map max 5", " V");
        configOutput(CV5_OUT, "CV 5");
        configOutput(GATE5_OUT, "Gate 5");
        configLight(GATE5_LED, "Gate LED");
        
        configParam(MIN6_PARAM, -10.f, 10.f, -10.f, "Map min 6", " V");
        configParam(MAX6_PARAM, -10.f, 10.f, 10.f, "Map max 6", " V");
        configOutput(CV6_OUT, "CV 6");
        configOutput(GATE6_OUT, "Gate 6");
        configLight(GATE6_LED, "Gate LED");
        
        configSwitch(PLAY_PARAM, 0, 1, 0, "Play/Stop");
        configSwitch(REC_PARAM, 0, 1, 0, "Rec Loop");
        configSwitch(ERASE_PARAM, 0, 1, 0, "Delete Loop");

	}

    void process(const ProcessArgs& args) override {
   
      //----------------leggo valori UI---------------//
       
        float macroCV = inputs[MACRO_IN].getVoltage();
        float macroKnob = params[MACRO_PARAM].getValue() + macroCV;
        float macroPot = 0;
        
        if (play) {
            macroPot = rack::math::clamp(macroPlay + macroKnob, -10.f, 10.f); //+macroKnob per avere gen offset
        }
        else {
            macroPot = rack::math::clamp(macroKnob, -10.f, 10.f);
        }
        
        for (int i = 0; i < NUM_CONTROLLI; i++) {
            potMin[i] = params[i].getValue();
            potMax[i] = params[i + 6].getValue();
            outCV[i] = mappaCV(macroPot, potMin[i], potMax[i]);
                }
        
        bool buttPlay = params[PLAY_PARAM].getValue();
        bool buttRec = params[REC_PARAM].getValue();
        bool buttErase = params[ERASE_PARAM].getValue();
    
        bool inPlay = inputs[PLAY_IN].getVoltage() >= 1.f;
        bool inRec = inputs[REC_IN].getVoltage() >= 1.f;
        
        //----------gestione GATE---------//
        
        //normalizzo in [0, 1]
        float valNorm = (macroPot + 10.f) * 0.05f;              // (val+10) / 20
        //divido in 6 zone
        int indiceAttivo = static_cast<int>(valNorm * 6.f);     // 0 … 5, con t=1 diventa 6
        if (indiceAttivo == 6) indiceAttivo = 5;                //estremo +10
        
        for (int i = 0; i < NUM_CONTROLLI; i++) {
            outGATE[i] = (i == indiceAttivo) ? 1 : 0;
            lights[i].setSmoothBrightness(outGATE[i], 5e-6f);
        }
        
      //-----------------gestione Registrazione------------------//
        
        if (recDetector.process(buttRec) || recGateDetector.process(inRec)) {
            rec = !rec;
            if (rec) {
                play = false;
                contaFrame = 0;
            }
        }
       if (playDetector.process(buttPlay) || playGateDetector.process(inPlay)) {
            play = !play;
            if (play) {
                rec = false;
                contaFrame = 0;
                indicePlay = 0;
                samplePlay = sampleRec; //qua ho un problema
                if (indiceRec == 0) play = false; // niente da riprodurre
                }
            }
        
        //----ERASE
        if (eraseDetector.process(buttErase)) {
            rec  = false;
            play = false;
            indiceRec = 0;
            indicePlay = 0;
            memset(bufferRec, 0, sizeof(bufferRec)); //azzero buffer
            macroPlay = 0;
            luceErase.trigger(0.1);
        }
        erase = luceErase.process(args.sampleTime);
        
        //----Rec
        if (rec) {
            if (++contaFrame >= sampleRec) {
                contaFrame = 0;
                bufferRec[indiceRec] = macroPot;
                indiceRec++;
                if (indiceRec >= 512) {   // buffer pieno, riparto
                    indiceRec = 0;
                }
             }
          }
        //---Play
        if (play) {
            if (++contaFrame >= samplePlay) {
            contaFrame = 0;
            float macroPlayback = bufferRec[indicePlay++];
            macroPlay = macroPlayback;
               if (indicePlay >= indiceRec) {
                indicePlay = 0;
                }
              }
           }
        //----Luci
        lights[PLAY_LED].setBrightness(play);
        lights[REC_LED].setBrightness(rec);
        lights[ERASE_LED].setBrightness(erase);
        
      //---------------------GEN OUTPUT-----------------------//

        for (int i = 0; i < NUM_CONTROLLI; i++) {
            
            if (outputs[i].isConnected()) {
                outputs[i].setVoltage(outCV[i]); //CV mappata out
                }
            if (outputs[i + 6].isConnected()) {
                outputs[i + 6].setVoltage(outGATE[i] * 10.f); //gate out
            }
        }

	 }
}; //end struct module

//------------definisco costruttore etichetta label
namespace {
struct MiniTextField : TextField {
    MiniTextField() {
        box.size = (mm2px(Vec(40, 7)));
       // box.Color   = nvgRGB(0x28, 0x2c, 0x34);   // non esiste per TextField
       // textColor  = nvgRGB(0xf8, 0xf8, 0xf2);   // stessa cosa
       // fontSize = 10.f;
        multiline = false;
        placeholder = " ...type label here...";
    }

  };
}

struct MacroWidget : ModuleWidget {
    MiniTextField* txt = nullptr;
    MacroWidget(Macro* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Macro.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(40.64, 44.0)), module, Macro::MACRO_PARAM));
        
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(10.5, 82.0)), module, Macro::MAX1_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(10.5, 94.0)), module, Macro::MIN1_PARAM));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.5, 106.0)), module, Macro::CV1_OUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.5, 118.0)), module, Macro::GATE1_OUT));
        addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(10.5, 112.5)), module, Macro::GATE1_LED));
        
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(22.5, 82.0)), module, Macro::MAX2_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(22.5, 94.0)), module, Macro::MIN2_PARAM));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(22.5, 106.0)), module, Macro::CV2_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(22.5, 118.0)), module, Macro::GATE2_OUT));
        addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(22.5, 112.5)), module, Macro::GATE2_LED));
        
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(34.5, 82.0)), module, Macro::MAX3_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(34.5, 94.0)), module, Macro::MIN3_PARAM));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(34.5, 106.0)), module, Macro::CV3_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(34.5, 118.0)), module, Macro::GATE3_OUT));
        addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(34.5, 112.5)), module, Macro::GATE3_LED));
        
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(46.5, 82.0)), module, Macro::MAX4_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(46.5, 94.0)), module, Macro::MIN4_PARAM));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(46.5, 106.0)), module, Macro::CV4_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(46.5, 118.0)), module, Macro::GATE4_OUT));
        addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(46.5, 112.5)), module, Macro::GATE4_LED));
        
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(58.5, 82.0)), module, Macro::MAX5_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(58.5, 94.0)), module, Macro::MIN5_PARAM));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(58.5, 106.0)), module, Macro::CV5_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(58.5, 118.0)), module, Macro::GATE5_OUT));
        addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(58.5, 112.5)), module, Macro::GATE5_LED));
        
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(70.5, 82.0)), module, Macro::MAX6_PARAM));
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(70.5, 94.0)), module, Macro::MIN6_PARAM));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(70.5, 106.0)), module, Macro::CV6_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(70.5, 118.0)), module, Macro::GATE6_OUT));
        addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(70.5, 112.5)), module, Macro::GATE6_LED));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.5, 44.0)), module, Macro::MACRO_IN));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(70.5, 64.0)), module, Macro::PLAY_IN));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(70.5, 44.0)), module, Macro::REC_IN));
        
        addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<GreenLight>>>(mm2px(Vec(70.5, 54.0)), module, Macro::PLAY_PARAM, Macro::PLAY_LED));
        addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<RedLight>>>(mm2px(Vec(70.5, 34.0)), module, Macro::REC_PARAM, Macro::REC_LED));
        addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(70.5, 24.0)), module, Macro::ERASE_PARAM, Macro::ERASE_LED));

        
        txt = createWidget<MiniTextField>(mm2px(Vec(20.813, 18.0)));
        addChild(txt);
        if (module)                                   // importa il testo salvato
            txt->setText(static_cast<Macro*>(module)->noteText);
              
	}
    void step() override {
        auto* m = dynamic_cast<Macro*>(module);
        if (m && txt) {
            m->noteText = txt->text; // da Widget a Modulo
        }
        ModuleWidget::step();
    }
};

Model* modelMacro = createModel<Macro, MacroWidget>("Macro");
