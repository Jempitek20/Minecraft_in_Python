#include <M5Cardputer.h>
#include <Preferences.h>
#include <math.h>
static constexpr int W=240,H=135,POP=24,IN=4,HID=6;
static constexpr float BX=45.0f,PW=18.0f,GRAV=.22f,FLAP=-3.8f,SPEED=1.8f;
struct Brain{float w1[IN][HID],b1[HID],w2[HID],b2,fitness;}; Brain pop[POP],champion{}; Preferences prefs;
float by,bv,px,gap; int score,generation=1,bestScore; bool running=false,training=true;
float frand(float a,float b){return a+(float)random(100000)/100000.0f*(b-a);} float sigmoid(float x){return 1.0f/(1.0f+expf(-x));}
void randomBrain(Brain &b){for(int i=0;i<IN;i++)for(int j=0;j<HID;j++)b.w1[i][j]=frand(-2,2);for(int j=0;j<HID;j++){b.b1[j]=frand(-2,2);b.w2[j]=frand(-2,2);}b.b2=frand(-2,2);b.fitness=0;}
float think(const Brain &b){float x[IN]={by/H,bv/5,(px-BX)/W,(gap-by)/H},h[HID];for(int j=0;j<HID;j++){float s=b.b1[j];for(int i=0;i<IN;i++)s+=x[i]*b.w1[i][j];h[j]=tanhf(s);}float o=b.b2;for(int j=0;j<HID;j++)o+=h[j]*b.w2[j];return sigmoid(o);}
void mutate(Brain &b,float a){for(int i=0;i<IN;i++)for(int j=0;j<HID;j++)if(frand(0,1)<.18)b.w1[i][j]+=frand(-a,a);for(int j=0;j<HID;j++){if(frand(0,1)<.18)b.b1[j]+=frand(-a,a);if(frand(0,1)<.18)b.w2[j]+=frand(-a,a);}if(frand(0,1)<.18)b.b2+=frand(-a,a);}
void resetGame(){by=67;bv=0;px=W;gap=frand(38,96);score=0;}
bool crashed(){if(by<5||by>H-5)return true;if(px+PW>=BX-5&&px<=BX+5&&(by<gap-24||by>gap+24))return true;return false;}
void drawGame(){auto &d=M5Cardputer.Display;d.fillScreen(TFT_BLACK);d.fillRect((int)px,0,(int)PW,max(0,(int)(gap-24)),TFT_GREEN);int y=(int)(gap+24);if(y<H)d.fillRect((int)px,y,(int)PW,H-y,TFT_GREEN);d.fillCircle((int)BX,(int)by,5,TFT_YELLOW);d.setTextColor(TFT_WHITE);d.setCursor(4,4);d.printf("GEN %d SCORE %d",generation,score);d.setCursor(4,16);d.printf("BEST %d",bestScore);}
float simulate(Brain &b,bool render){resetGame();unsigned long start=millis();while(millis()-start<12000){if(think(b)>.5f)bv=FLAP;bv+=GRAV;by+=bv;px-=SPEED;if(px<-PW){px=W;gap=frand(38,96);score++;}if(crashed()){b.fitness=score*1000.0f+(millis()-start)*.02f;return b.fitness;}if(render){drawGame();delay(8);}yield();}b.fitness=score*1000.0f+10000;return b.fitness;}
void evolve(){int bi=0;for(int i=1;i<POP;i++)if(pop[i].fitness>pop[bi].fitness)bi=i;champion=pop[bi];bestScore=max(bestScore,(int)(champion.fitness/1000));Brain next[POP];next[0]=champion;for(int i=1;i<POP;i++){next[i]=champion;mutate(next[i],.45f);}for(int i=0;i<POP;i++)pop[i]=next[i];generation++;}
void saveChampion(){prefs.begin("neuroflappy",false);prefs.putBytes("brain",&champion,sizeof(champion));prefs.end();}
void setup(){auto cfg=M5.config();M5Cardputer.begin(cfg,true);M5Cardputer.Display.setRotation(1);randomSeed(esp_random());for(auto &b:pop)randomBrain(b);champion.fitness=-1;}
void loop(){M5Cardputer.update();auto &k=M5Cardputer.Keyboard;if(k.isChange()&&k.isPressed()){auto ks=k.keysState();for(char c:ks.word){if(c=='q')running=false;if(c=='t'){training=true;running=true;}if(c=='p'){training=false;running=true;}}}if(!running){M5Cardputer.Display.fillScreen(TFT_BLACK);M5Cardputer.Display.setTextColor(TFT_WHITE);M5Cardputer.Display.setCursor(8,20);M5Cardputer.Display.printf("NEURO FLAPPY");M5Cardputer.Display.setCursor(8,42);M5Cardputer.Display.printf("T TRAIN  P PLAY");M5Cardputer.Display.setCursor(8,58);M5Cardputer.Display.printf("Q STOP");delay(20);return;}if(training){for(int i=0;i<POP;i++)simulate(pop[i],false);evolve();if(generation%5==0)saveChampion();}else{simulate(champion,true);delay(300);}}
