
// Standard arduino setup function ---------------------------------------------
void setup() {
    Serial.begin(115200);
}


// Standard Arduino loop function. This gets repeatedly called at high rate ----
void loop()
{
    static char d;
    if ( Serial.available() )
    {
        d = Serial.read();
//        delay(3);  
        Serial.write(d + 1); 
    }
}

