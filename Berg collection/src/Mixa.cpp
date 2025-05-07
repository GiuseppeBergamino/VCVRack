#include "plugin.hpp"

#define NUM_CONTROLLI 6


struct Mixa : Module {
	enum ParamId {
        VOL1_PARAM,
        VOL2_PARAM,
        VOL3_PARAM,
        VOL4_PARAM,
        VOL5_PARAM,
        VOL6_PARAM,
        BUTT1_PARAM,
        BUTT2_PARAM,
        BUTT3_PARAM,
        BUTT4_PARAM,
        BUTT5_PARAM,
        BUTT6_PARAM,
        VOL_MASTER_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		UNO_IN,
        DUE_IN,
        TRE_IN,
        QUA_IN,
        CIN_IN,
        SEI_IN,
        BUTT1_IN,
        BUTT2_IN,
        BUTT3_IN,
        BUTT4_IN,
        BUTT5_IN,
        BUTT6_IN,
        VCA1_IN,
        VCA2_IN,
        VCA3_IN,
        VCA4_IN,
        VCA5_IN,
        VCA6_IN,
		INPUTS_LEN
	};
	enum OutputId {
        VCA1_OUT,
        VCA2_OUT,
        VCA3_OUT,
        VCA4_OUT,
        VCA5_OUT,
        VCA6_OUT,
		MASTER_OUT,
		OUTPUTS_LEN
	};
    enum LightsId {
        GATE1_LED,
        GATE2_LED,
        GATE3_LED,
        GATE4_LED,
        GATE5_LED,
        GATE6_LED,
        LIGHTS_LEN
    };
    
    float potVol[NUM_CONTROLLI] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
    float cvVol[NUM_CONTROLLI] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
    
    dsp::SchmittTrigger butt1Detector;
    dsp::SchmittTrigger butt2Detector;
    dsp::SchmittTrigger butt3Detector;
    dsp::SchmittTrigger butt4Detector;
    dsp::SchmittTrigger butt5Detector;
    dsp::SchmittTrigger butt6Detector;
  
    Mixa() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        
		configParam(VOL_MASTER_PARAM, 0.f, 1.5f, 1.f, "Master Volume");
        
        configParam(VOL1_PARAM, 0.f, 1.5f, 0.f, "Volume 1");
        configParam(VOL2_PARAM, 0.f, 1.5f, 0.f, "Volume 2");
        configParam(VOL3_PARAM, 0.f, 1.5f, 0.f, "Volume 3");
        configParam(VOL4_PARAM, 0.f, 1.5f, 0.f, "Volume 4");
        configParam(VOL5_PARAM, 0.f, 1.5f, 0.f, "Volume 5");
        configParam(VOL6_PARAM, 0.f, 1.5f, 0.f, "Volume 6");
        
        configSwitch(BUTT1_PARAM, 0, 1, 1, "Mute 1");
        configSwitch(BUTT2_PARAM, 0, 1, 1, "Mute 2");
        configSwitch(BUTT3_PARAM, 0, 1, 1, "Mute 3");
        configSwitch(BUTT4_PARAM, 0, 1, 1, "Mute 4");
        configSwitch(BUTT5_PARAM, 0, 1, 1, "Mute 5");
        configSwitch(BUTT6_PARAM, 0, 1, 1, "Mute 6");
        
        configInput(UNO_IN, "Ch 1");
        configInput(DUE_IN, "Ch 2");
        configInput(TRE_IN, "Ch 3");
        configInput(QUA_IN, "Ch 4");
        configInput(CIN_IN, "Ch 5");
        configInput(SEI_IN, "Ch 6");
        
        configInput(BUTT1_IN, "Mute 1");
        configInput(BUTT2_IN, "Mute 2");
        configInput(BUTT3_IN, "Mute 3");
        configInput(BUTT4_IN, "Mute 4");
        configInput(BUTT5_IN, "Mute 5");
        configInput(BUTT6_IN, "Mute 6");
        
        configInput(VCA1_IN, "VCA 1");
        configInput(VCA2_IN, "VCA 2");
        configInput(VCA3_IN, "VCA 3");
        configInput(VCA4_IN, "VCA 4");
        configInput(VCA5_IN, "VCA 5");
        configInput(VCA6_IN, "VCA 6");
        
        configOutput(VCA1_OUT, "VCA 1");
        configOutput(VCA2_OUT, "VCA 2");
        configOutput(VCA3_OUT, "VCA 3");
        configOutput(VCA4_OUT, "VCA 4");
        configOutput(VCA5_OUT, "VCA 5");
        configOutput(VCA6_OUT, "VCA 6");
        
        configOutput(MASTER_OUT, "Master");
        
	}

    void process(const ProcessArgs& args) override {
        
        //----------------leggo valori UI---------------//
      

	 }
}; //end struct module


struct MixaWidget : ModuleWidget {
    
    MixaWidget(Mixa* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Mixa.svg")));
        
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        
        addParam(createParamCentered<RoundHugeBlackKnob>(mm2px(Vec(40.64, 56.84)), module, Mixa::VOL_MASTER_PARAM));
        
        addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<RedLight>>>(mm2px(Vec(40.64, 36.94)), module, Mixa::BUTT1_PARAM, Mixa::GATE1_LED));
        addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(40.64, 26.97)), module, Mixa::VOL1_PARAM));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(40.64, 17.25)), module, Mixa::UNO_IN));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(31.02, 28.56)), module, Mixa::BUTT1_IN));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(32.43, 20.04)), module, Mixa::VCA1_IN));



        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(63.53, 109.42)), module, Mixa::MASTER_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(63.53, 97.05)), module, Mixa::VCA1_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(74.42, 103.54)), module, Mixa::VCA2_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(74.07, 115.91)), module, Mixa::VCA3_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(63.18, 121.79)), module, Mixa::VCA4_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(52.64, 115.29)), module, Mixa::VCA5_OUT));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(53.00, 102.93)), module, Mixa::VCA6_OUT));

	}

};

Model* modelMixa = createModel<Mixa, MixaWidget>("Mixa");
