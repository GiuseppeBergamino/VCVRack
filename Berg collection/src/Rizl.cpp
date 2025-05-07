#include "plugin.hpp"
#include "SVF.hpp"


struct Rizl : Module {
	enum ParamId {
        DELTA_PARAM,
        FREQ_PARAM,
        RESO_PARAM,
        FM_PARAM,
        MIX_PARAM,
		PARAMS_LEN
	};
	enum InputId {
        DELTA_IN,
		MAIN_IN,
        FREQ_IN,
        RESO_IN,
        FM_IN,
        VCA_IN,
        CV_MIX_IN,
        CV_FM_IN,
		INPUTS_LEN
	};
	enum OutputId {
        LP_OUT,
        BP_OUT,
        HP_OUT,
        MIX_OUT,
		OUTPUTS_LEN
	};

    SVF<float> *filtro1 = new SVF<float>(100, 0.1);
    float hpf1, bpf1, lpf1;
    
    SVF<float> *filtro2LP = new SVF<float>(100, 0.1);
    float hpf2LP, bpf2LP, lpf2LP;
    
    SVF<float> *filtro2BP = new SVF<float>(100, 0.1);
    float hpf2BP, bpf2BP, lpf2BP;
    
    SVF<float> *filtro2HP = new SVF<float>(100, 0.1);
    float hpf2HP, bpf2HP, lpf2HP;
    
    rack::dsp::SlewLimiter slewMix;

    Rizl() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN);
        
        configParam(DELTA_PARAM, -1.f, 1.f, 0.f, "Δ");
        float freqCampionamento = APP->engine->getSampleRate();
		configParam(FREQ_PARAM, 20.f, freqCampionamento / 6.f, freqCampionamento / 6.f, "Frequency", " Hz");
        configParam(RESO_PARAM, 1e-6, 1.f, 1e-6, "Resonance", " %", 0, 100);
        configParam(FM_PARAM, -1.f, 1.f, 0.f, "FM Amount", " %", 0, 100);
        
        configParam(MIX_PARAM, 0.f, 1.f, 0.f, "Type Lp<->Bp<->Hp");
        
        configInput(DELTA_IN, "Δ CV");
        configInput(MAIN_IN, "Main");
        configInput(FREQ_IN, "Frequency CV");
        configInput(RESO_IN, "Resonance CV");
        configInput(FM_IN, "FM");
        configInput(CV_FM_IN, "FM Amount");
        configInput(VCA_IN, "VCA");
        configInput(CV_MIX_IN, "Filter Type CV");
  
        configOutput(LP_OUT, "Low Pass");
        configOutput(BP_OUT, "Band Pass");
        configOutput(HP_OUT, "High Pass");
        configOutput(MIX_OUT, "Filter Out");
        
        slewMix.setRiseFall(0.015f, 0.015f);
        
	}

    void process(const ProcessArgs& args) override {
        
     //----------------leggo valori UI---------------//
        float mainIn = inputs[MAIN_IN].getVoltage() * 0.15f; //porto tra -1 e 1, 0.16 = 0.2 * 0.8
        
        float fc = params[FREQ_PARAM].getValue();
        if (inputs[FREQ_IN].isConnected()) {
            fc *= std::pow(2.f, inputs[FREQ_IN].getVoltage() * 4.f / 12.f);
        }
        float fc1 = fc * std::pow(2.f, (rack::random::uniform() - 0.5f) * 0.2f / 12.f);
        float fc2 = fc;
        
        float resoPot = params[RESO_PARAM].getValue();
        float resoCV = inputs[RESO_IN].getVoltage() * 0.1;
        float resoVal = resoPot + resoCV;
        resoVal = rack::math::clamp(resoVal, 1e-6, 1.f);
        float damp = 1.f / (resoVal * 20.f);
        float damp1 = 5.f + (rack::random::uniform() - 0.5) * 0.02;
        float damp2 = damp;
    
        if (inputs[FM_IN].isConnected()) {
            float fmPot = params[FM_PARAM].getValue();
            float fmCV = inputs[CV_FM_IN].getVoltage() * 0.1;
            float fmVal = fmPot + fmCV;
            float modulanteIn = inputs[FM_IN].getVoltage() * fmVal;
            
            fc1 *= std::pow(2.f, modulanteIn * 0.99f);
            fc2 *= std::pow(2.f, modulanteIn);
        }
        
        float mixPot = params[MIX_PARAM].getValue();
        float mixCV = inputs[CV_MIX_IN].getVoltage() * 0.1f;
        mixCV = rack::math::clamp(mixCV, -1.f, 1.f);
        float mixControl = slewMix.process(0.015f, mixPot + mixCV);
        
        float inVCA = inputs[VCA_IN].getVoltage() * 0.1f;
        inVCA = rack::math::clamp(inVCA, -1.f, 1.f);
        
        float deltaPot = params[DELTA_PARAM].getValue();
        float deltaCV = inputs[DELTA_IN].getVoltage() * 0.1f;
        float delta = deltaPot + deltaCV;
        delta = rack::math::clamp(delta, -2.f, 2.f);
    
        
     //----------gestione Filtro---------//
        
        filtro1->setCoeffs(fc1, damp1);
        filtro1->process(mainIn, &hpf1, &bpf1, &lpf1);
        
        filtro2LP->setCoeffs(fc2 + fc2 * 0.25f * delta, damp2);
        filtro2LP->process(lpf1, &hpf2LP, &bpf2LP, &lpf2LP);
        
        filtro2BP->setCoeffs(fc2 * 0.5f + fc2 * 0.25f * delta, damp2);
        filtro2BP->process(bpf1, &hpf2BP, &bpf2BP, &lpf2BP);
        
        filtro2HP->setCoeffs(fc2 * 0.5f + fc2 * 0.25f * delta, damp2);
        filtro2BP->process(hpf1, &hpf2HP, &bpf2HP, &lpf2HP);
        
        //tanh non va bene, fa aliasing, devo compensare il volume rispetto alla resonance
        float LowPassOut  = lpf2LP * (1.f - (resoVal * 0.5f)) + lpf1 * (resoVal * 0.5f);
        LowPassOut = rack::math::clamp(LowPassOut, -2.f, 2.f);
        
        float BandPassOut = bpf2BP * (1.f - (resoVal * 0.5f)) + lpf2BP * (resoVal * 0.75f); //lpf2BP
        BandPassOut = rack::math::clamp(BandPassOut, -2.f, 2.f);
        
        float HighPassOut = hpf2HP * (1.f - (resoVal * 0.5f)) + bpf1 * (resoVal * 0.25f);
        
        /*
        float LowPassOut = lpf1;
        float BandPassOut = hpf1 - bpf1; //lpf1 + hpf1 =  notch
        float HighPassOut = hpf1; */
        
      //---------gestione MIX---------//
        
        float mixOut;
        
        if (mixControl < 0.5f) {
            float cross = mixControl * 2.f; //riscalo 0-0.5 su 0-1
            mixOut = rack::math::crossfade(LowPassOut, BandPassOut, cross);
        }
        else {
            float cross = (mixControl - 0.5f) * 2.f; //riscalo 0.5-1 su 0-1
            mixOut = rack::math::crossfade(BandPassOut, HighPassOut, cross);
        }
        
      //------------gestione VCA------//
        
        float gainVCA = 1.f; //se non è connesso il gain è unitario
        if (inputs[VCA_IN].isConnected()) {
            gainVCA = rack::math::clamp(inVCA, -1.f, 1.f);
         }
        
         mixOut *= gainVCA;

     //---------------------GEN OUTPUT-----------------------//
        
        if (outputs[LP_OUT].isConnected()) {
            outputs[LP_OUT].setVoltage(5.f * LowPassOut);
        }
        
        if (outputs[BP_OUT].isConnected()) {
            outputs[BP_OUT].setVoltage(5.f * BandPassOut);
        }
        
        if (outputs[HP_OUT].isConnected()) {
            outputs[HP_OUT].setVoltage(5.f * HighPassOut);
        }
        
        if (outputs[MIX_OUT].isConnected()) {
            outputs[MIX_OUT].setVoltage(5.f * mixOut);
        }
        
    }
}; //end struct module


    struct RizlWidget : ModuleWidget {
        RizlWidget(Rizl* module) {
            setModule(module);
            setPanel(createPanel(asset::plugin(pluginInstance, "res/Rizl.svg")));
            
            addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
            addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
            addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
            addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
            
            addParam(createParamCentered<RoundBigBlackKnob>(mm2px(Vec(25.4, 57.61)), module, Rizl::FREQ_PARAM));
            addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(8.467, 48.249)), module, Rizl::DELTA_PARAM));
            addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(31.4, 38.5)), module, Rizl::RESO_PARAM));
            addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(42.33, 67.845)), module, Rizl::FM_PARAM));
            addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(25.4, 106.0)), module, Rizl::MIX_PARAM));
            
            addInput(createInputCentered<PJ301MPort>(mm2px(Vec(19.175, 22.0)), module, Rizl::FREQ_IN));
            addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.467, 22.0)), module, Rizl::DELTA_IN));
            addInput(createInputCentered<PJ301MPort>(mm2px(Vec(31.41, 22.0)), module, Rizl::RESO_IN));
            addInput(createInputCentered<PJ301MPort>(mm2px(Vec(42.333, 22.0)), module, Rizl::FM_IN));
            addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.1, 94.0)), module, Rizl::CV_FM_IN));
            addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.467, 82.0)), module, Rizl::MAIN_IN));
            
            addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.4, 94.0)), module, Rizl::CV_MIX_IN));
            addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.7, 106.0)), module, Rizl::VCA_IN));
    
            addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(38.1, 106.0)), module, Rizl::MIX_OUT));
            addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(12.7, 118.0)), module, Rizl::LP_OUT));
            addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(25.4, 118.0)), module, Rizl::BP_OUT));
            addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(38.1, 118.0)), module, Rizl::HP_OUT));
            

        }

};

Model* modelRizl = createModel<Rizl, RizlWidget>("Rizl");
