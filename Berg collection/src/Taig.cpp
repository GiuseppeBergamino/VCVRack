#include "plugin.hpp"

struct Taig : Module {
	enum ParamId {
        BPM_PARAM,
        DIV_MULT_PARAM,
        BUTTON_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		GATE_IN,
        CV_BPM_IN,
        CLOCK_IN,
		INPUTS_LEN
	};
	enum OutputId {
        TRIG_OUT,
        END_GATE_OUT,
		OUTPUTS_LEN
	};
    enum LightsId {
        TRIG_LED,
        END_GATE_LED,
        BUTTON_LED,
        LIGHTS_LEN
    };
        
    bool switchGate = false;
    bool trigger;
    
    dsp::SchmittTrigger buttonDetector;
    dsp::SchmittTrigger gateEndDetector;
    
    dsp::PulseGenerator intClock;
    dsp::PulseGenerator intGate;
    bool genClock = 0;
    bool genGate = 0;
    float clockLenght;
    
    dsp::SchmittTrigger extClockDetector;
    float tempoExt = 1.f;
    int contaExt = 0;
    dsp::PulseGenerator externalClock;
    dsp::PulseGenerator externalGate;
    bool genExtClock = 0;
    bool genExtGate = 0;
    bool isExtConnesso = 0;
    
    dsp::PulseGenerator endOfGate;

    Taig() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        
		configParam(BPM_PARAM, 40.f, 240.f, 120.f, "BPM"); //qua va cambiato in base alla presenza di extClock
        configParam(DIV_MULT_PARAM, 0.2f, 5.f, 1.f, "Clock Mult/Div");
        
        configSwitch(BUTTON_PARAM, 0, 1, 0, "Invert Gate");

        configInput(GATE_IN, "Gate");
        configInput(CV_BPM_IN, "CV Tempo");
        configInput(CLOCK_IN, "Ext Clock");

        configOutput(TRIG_OUT, "Trig");
        configOutput(END_GATE_OUT, "End of Gate");
    
        configLight(TRIG_LED, "Trig");
        configLight(END_GATE_LED, "End of Gate");
        
	}

    void process(const ProcessArgs& args) override {
        
        //----------------leggo valori UI---------------//
         
        bool gate;
        bool buttGate = params[BUTTON_PARAM].getValue();
        
        if (buttonDetector.process(buttGate)) {
            switchGate = !switchGate;
          }
        
        bool gateIn = inputs[GATE_IN].getVoltage() >= 1.f;
        gate = switchGate ? !gateIn : gateIn;
        
        
        bool extClock = inputs[CLOCK_IN].getVoltage() >= 1.f;
        
        float bpmPot = params[BPM_PARAM].getValue();
        float bpmCV  = inputs[CV_BPM_IN].getVoltage() * 6.f; //aggiustare range, per ora [-60, 60]
        float BPM = bpmPot + bpmCV;
        BPM = rack::math::clamp(BPM, 20.f, 300.f);

    
        float divMultPot = params[DIV_MULT_PARAM].getValue();
        float divMultCV  = inputs[CV_BPM_IN].getVoltage() * 0.5f; //aggiustare range, ora [-2, 2]
        float divMult = divMultPot + divMultCV;
        divMult = rack::math::clamp(divMult, 0.1f, 10.f);
        
        isExtConnesso = inputs[CLOCK_IN].isConnected();
        
        //----------gestione Tempo-----------//
        //-----Clock interno
        clockLenght = 60.f / BPM;
        
        if (genClock == 0 && genGate == 0) {
            intClock.trigger(1e-2);
            intGate.trigger(clockLenght);
        }
        genClock = intClock.process(args.sampleTime);
        genGate = intGate.process(args.sampleTime);
        
        
        //-----Clock esterno
        if(isExtConnesso) {

            if(extClockDetector.process(extClock)) {
                tempoExt = contaExt * args.sampleTime;
                contaExt = 0;
                
                if (genExtClock == 0 && genExtGate == 0) {
                     externalClock.trigger(1e-2);
                     externalGate.trigger(tempoExt * divMult);
                }
            }
            contaExt++;
            
            if (genExtClock == 0 && genExtGate == 0) {
                 externalClock.trigger(1e-2);
                 externalGate.trigger(tempoExt * divMult);
            }

            genExtClock = externalClock.process(args.sampleTime);
            genExtGate = externalGate.process(args.sampleTime);
            
            trigger = gate * genExtClock; //output se ho clock esterno
        }
        
        else {
         externalClock.reset();
         externalGate.reset();
         contaExt = 1.f;
         genExtClock = 0;
         genExtGate = 0;
            
         trigger = gate * genClock; //output se ho clock interno
            
        }

        //----------gestione end of gate---------//
        
            
         if (gateEndDetector.process(!gate)) {
            endOfGate.trigger(1e-2);
          }
         bool fineGate = endOfGate.process(args.sampleTime);
    
        
        //--------------Luci---------------//

        lights[BUTTON_LED].setBrightness(switchGate);

        lights[TRIG_LED].setSmoothBrightness(trigger,5e-6f );
        lights[END_GATE_LED].setSmoothBrightness(fineGate, 5e-6f);

        
      //---------------------GEN OUTPUT-----------------------//
        
        outputs[TRIG_OUT].setVoltage(trigger * 10.f);
        outputs[END_GATE_OUT].setVoltage(fineGate * 10.f);

        
    }
}; //end struct module



    struct TaigWidget : ModuleWidget {
        ParamWidget* bpmParam;
        ParamWidget* divMultParam;
        TaigWidget(Taig* module) {
            setModule(module);
            //setPanel(createPanel(asset::plugin(pluginInstance, "res/Taig.svg")));
            setPanel(createPanel(asset::plugin(pluginInstance, "res/Taig.svg"), asset::plugin(pluginInstance, "res/Taig-dark.svg")));
            
            addChild(createWidget<ScrewSilver>(Vec(box.size.x - 4 * RACK_GRID_WIDTH, 0)));
            addChild(createWidget<ScrewSilver>(Vec(box.size.x - 1 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
            
           
           addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(mm2px(Vec(15.2, 23.01)), module, Taig::BUTTON_PARAM, Taig::BUTTON_LED));
            addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.0, 23.01)), module, Taig::GATE_IN));
          
            addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.1, 35.01)), module, Taig::CV_BPM_IN));
            
            //addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.1, 55.263)), module, Taig::BPM_PARAM));
            bpmParam = createParamCentered<RoundBlackKnob>(mm2px(Vec(10.1, 55.263)), module, Taig::BPM_PARAM);
            addParam(bpmParam);
            divMultParam = createParamCentered<RoundBlackKnob>(mm2px(Vec(10.1, 55.263)), module, Taig::DIV_MULT_PARAM);
            divMultParam->hide();
            addParam(divMultParam);
            
            
            addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.1, 72.0)), module, Taig::CLOCK_IN));
            
            addChild(createLightCentered<SmallLight<GreenLight>>(mm2px(Vec(4.5, 106.0)), module, Taig::TRIG_LED));
            addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.1, 106.0)), module, Taig::TRIG_OUT));
            
            addChild(createLightCentered<SmallLight<RedLight>>(mm2px(Vec(4.5, 118.0)), module, Taig::END_GATE_LED));
            addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(10.1, 118.0)), module, Taig::END_GATE_OUT));

        
        }
        
        void step() override {
            Taig* module = dynamic_cast<Taig*>(this->module);

            if (module) {
                bpmParam->visible = (module->isExtConnesso == 0);
                divMultParam->visible = (module->isExtConnesso == 1);
            }

            ModuleWidget::step();
        }

};

Model* modelTaig = createModel<Taig, TaigWidget>("Taig");
