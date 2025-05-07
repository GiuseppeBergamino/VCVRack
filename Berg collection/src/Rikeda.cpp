#include "plugin.hpp"

float genRandom(float valMin, float valMax) {
    return valMin + (valMax - valMin) * rack::random::uniform();
}

struct Rikeda : Module {
	enum ParamId {
        PROB1_PARAM,
        PROB2_PARAM,
        GATE_LENGHT1_PARAM,
        GATE_LENGHT2_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		TRIG1_IN,
        TRIG2_IN,
        CV_LENGHT1_IN,
        CV_LENGHT2_IN,
        CV_PROB1_IN,
        CV_PROB2_IN,
		INPUTS_LEN
	};
	enum OutputId {
        GATE_TRUE1_OUT,
        GATE_FALSE1_OUT,
        GATE_TRUE2_OUT,
        GATE_FALSE2_OUT,
		OUTPUTS_LEN
	};
    enum LightsId {
        GATE_TRUE1_LED,
        GATE_FALSE1_LED,
        GATE_TRUE2_LED,
        GATE_FALSE2_LED,
        LIGHTS_LEN
    };
        
    bool outGateTrue1, outGateTrue2;
    bool outGateFalse1, outGateFalse2;
    
    dsp::SchmittTrigger trig1Detector;
    dsp::SchmittTrigger trig2Detector;

    dsp::PulseGenerator GateTrue1;
    dsp::PulseGenerator GateFalse1;
    dsp::PulseGenerator GateTrue2;
    dsp::PulseGenerator GateFalse2;


    Rikeda() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        
		configParam(PROB1_PARAM, 0.f, 1.f, 0.5f, "Probability 1 Out", " %", 0, 100);
        configParam(PROB2_PARAM, 0.f, 1.f, 0.5f, "Probability 2 Out", " %", 0, 100);
        
        configParam(GATE_LENGHT1_PARAM, 1e-2, 2.f, 0.15f, "Gate 1 Lenght", " s");
        configParam(GATE_LENGHT2_PARAM, 1e-2, 2.f, 0.15f, "Gate 2 Lenght", " s");
        
        configInput(TRIG1_IN, "Trigger 1");
        configInput(TRIG2_IN, "Trigger 2");
        configInput(CV_PROB1_IN, "Probability CV 1");
        configInput(CV_PROB2_IN, "Probability CV 2");
        configInput(CV_LENGHT1_IN, "Gate Lenght CV 1");
        configInput(CV_LENGHT2_IN, "Gate Lenght CV 2");

        configOutput(GATE_TRUE1_OUT, "Gate True 1");
        configOutput(GATE_FALSE1_OUT, "Gate False 1");
        configOutput(GATE_TRUE2_OUT, "Gate True 2");
        configOutput(GATE_FALSE2_OUT, "Gate False 2");
        
        configLight(GATE_TRUE1_LED, "Gate True LED");
        configLight(GATE_FALSE1_LED, "Gate False LED");
        configLight(GATE_TRUE2_LED, "Gate True LED");
        configLight(GATE_FALSE1_LED, "Gate False LED");
        
	}

    void process(const ProcessArgs& args) override {
        
        //----------------leggo valori UI---------------//
        
        float potProb1 = params[PROB1_PARAM].getValue();
        float potProb2 = params[PROB2_PARAM].getValue();
        float potLenght1 = params[GATE_LENGHT1_PARAM].getValue();
        float potLenght2 = params[GATE_LENGHT2_PARAM].getValue();
        
        float cvProb1 = inputs[CV_PROB1_IN].getVoltage() * 0.1f;
        float cvProb2 = inputs[CV_PROB2_IN].getVoltage() * 0.1f;
        float cvLenght1 = inputs[CV_LENGHT1_IN].getVoltage() * 0.2f + 1e-2;
        float cvLenght2 = inputs[CV_LENGHT2_IN].getVoltage() * 0.2f + 1e-2;
        
        float prob1 = potProb1 + cvProb1;
        float prob2 = potProb2 + cvProb2;
        
        float tempoGate1 = potLenght1 + cvLenght1;
        float tempoGate2 = potLenght2 + cvLenght2;
        
        tempoGate1 = rack::math::clamp(tempoGate1, 1e-2, 4.f);
        tempoGate2 = rack::math::clamp(tempoGate2, 1e-2, 4.f);
        prob1 = rack::math::clamp(prob1, 0.f, 1.f);
        prob2 = rack::math::clamp(prob2, 0.f, 1.f);
        
        bool inTrig1 = inputs[TRIG1_IN].getVoltage() >= 1.f;
        bool inTrig2 = inputs[TRIG2_IN].getVoltage() >= 1.f;
    
        
        //----------gestione GATE---------//
        
        if (trig1Detector.process(inTrig1)) {
            float random1 = rack::random::uniform();
            if (random1 <= prob1) {
                GateTrue1.trigger(tempoGate1);
            }
            else {
               GateFalse1.trigger(genRandom(1e-2, tempoGate1));
            }

        }
        outGateTrue1 = GateTrue1.process(args.sampleTime);
        outGateFalse1 = GateFalse1.process(args.sampleTime);
        
        if (trig2Detector.process(inTrig2)) {
            float random2 = rack::random::uniform();
            if (random2 <= prob2) {
                GateTrue2.trigger(tempoGate2);
            }
            else {
               GateFalse2.trigger(genRandom(1e-2, tempoGate2));
            }
        }
        outGateTrue2 = GateTrue2.process(args.sampleTime);
        outGateFalse2 = GateFalse2.process(args.sampleTime);

        //----Luci
        lights[GATE_TRUE1_LED].setBrightness(outGateTrue1);
        lights[GATE_TRUE2_LED].setBrightness(outGateTrue2);
        lights[GATE_FALSE1_LED].setBrightness(outGateFalse1);
        lights[GATE_FALSE2_LED].setBrightness(outGateFalse2);
        
      //---------------------GEN OUTPUT-----------------------//
        
            if (outputs[GATE_TRUE1_OUT].isConnected()) {
                outputs[GATE_TRUE1_OUT].setVoltage(outGateTrue1 * 10.f);
                }
            if (outputs[GATE_TRUE2_OUT].isConnected()) {
                outputs[GATE_TRUE2_OUT].setVoltage(outGateTrue2 * 10.f);
                }
            if (outputs[GATE_FALSE1_OUT].isConnected()) {
                outputs[GATE_FALSE1_OUT].setVoltage(outGateFalse1 * 10.f);
                }
            if (outputs[GATE_FALSE2_OUT].isConnected()) {
                outputs[GATE_FALSE2_OUT].setVoltage(outGateFalse2 * 10.f);
                }
        
    }
}; //end struct module


    struct RikedaWidget : ModuleWidget {
        RikedaWidget(Rikeda* module) {
            setModule(module);
            setPanel(createPanel(asset::plugin(pluginInstance, "res/Rikeda.svg")));
            
            addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
            addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
            addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
            addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
            
            addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(16.5, 38.0)), module,
                Rikeda::PROB1_PARAM));
            addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(34.471, 38.0)), module, Rikeda::PROB2_PARAM));
            addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(16.5, 60.0)), module, Rikeda::GATE_LENGHT1_PARAM));
            addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(34.471, 60.0)), module, Rikeda::GATE_LENGHT2_PARAM));
            
            addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.7, 22.0)), module, Rikeda::TRIG1_IN));
            addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.1, 22.0)), module, Rikeda::TRIG2_IN));
            addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.7, 91.0)), module, Rikeda::CV_PROB1_IN));
            addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.1, 91.0)), module, Rikeda::CV_PROB2_IN));
            addInput(createInputCentered<PJ301MPort>(mm2px(Vec(12.7, 79.0)), module, Rikeda::CV_LENGHT1_IN));
            addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.1, 79.0)), module, Rikeda::CV_LENGHT2_IN));
            
            addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(12.7, 106.0)), module, Rikeda::GATE_TRUE1_OUT));
            addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(12.7, 118.0)), module, Rikeda::GATE_FALSE1_OUT));
            addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(38.1, 106.0)), module, Rikeda::GATE_TRUE2_OUT));
            addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(38.1, 118.0)), module, Rikeda::GATE_FALSE2_OUT));
            
            addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(12.7, 100.5)), module, Rikeda::GATE_TRUE1_LED));
            addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(12.7, 112.5)), module, Rikeda::GATE_FALSE1_LED));
            addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(38.1, 100.5)), module, Rikeda::GATE_TRUE2_LED));
            addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(38.1, 112.5)), module, Rikeda::GATE_FALSE2_LED));
        }

};

Model* modelRikeda = createModel<Rikeda, RikedaWidget>("Rikeda");
