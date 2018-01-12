// Callback function to handle com objects updates
void knxEvents(byte index) {
switch (index)
  {
      case 60:  //Taster 11 
      if (Knx.read(60))    {Taster_0_19_Comand[11] = true;  Taster_0_19_State[11] = true; } 
      else                 {Taster_0_19_Comand[11] = true; Taster_0_19_State[11] = false;}
      #ifdef KDEBUG
      Debug.print("Taster_11");
      Debug.println("%d", Taster_0_19_State[11]);
      #endif
      break;

      case 61:  //Taster 12 
      if (Knx.read(61))    {Taster_0_19_Comand[12] = true;  Taster_0_19_State[12] = true; } 
      else                 {Taster_0_19_Comand[12] = true; Taster_0_19_State[12] = false;}
      #ifdef KDEBUG
      Debug.print("Taster_12");
      Debug.println("%d", Taster_0_19_State[12]);
      #endif
      break;

       case 62:  //Taster 13 
      if (Knx.read(62))    {Taster_0_19_Comand[13] = true;  Taster_0_19_State[13] = true; } 
      else                 {Taster_0_19_Comand[13] = true; Taster_0_19_State[13] = false;}
      #ifdef KDEBUG
      Debug.print("Taster_13");
      Debug.println("%d", Taster_0_19_State[13]);
      #endif
      break;

       case 63:  //Taster 14 
      if (Knx.read(63))    {Taster_0_19_Comand[14] = true;  Taster_0_19_State[14] = true; } 
      else                 {Taster_0_19_Comand[14] = true; Taster_0_19_State[14] = false;}
      #ifdef KDEBUG
      Debug.print("Taster_14");
      Debug.println("%d", Taster_0_19_State[14]);
      #endif
      break;

       case 64:  //Taster 15 
      if (Knx.read(64))    {Taster_0_19_Comand[15] = true;  Taster_0_19_State[15] = true; } 
      else                 {Taster_0_19_Comand[15] = true; Taster_0_19_State[15] = false;}
      #ifdef KDEBUG
      Debug.print("Taster_15");
      Debug.println("%d", Taster_0_19_State[15]);
      #endif
      break;

       case 65:  //Taster 16 
      if (Knx.read(65))    {Taster_0_19_Comand[16] = true;  Taster_0_19_State[16] = true; } 
      else                 {Taster_0_19_Comand[16] = true; Taster_0_19_State[16] = false;}
      #ifdef KDEBUG
      Debug.print("Taster_16");
      Debug.println("%d", Taster_0_19_State[16]);
      #endif
      break;

       case 66:  //Taster 17 
      if (Knx.read(66))    {Taster_0_19_Comand[17] = true;  Taster_0_19_State[17] = true; } 
      else                 {Taster_0_19_Comand[17] = true; Taster_0_19_State[17] = false;}
      #ifdef KDEBUG
      Debug.print("Taster_17");
      Debug.println("%d", Taster_0_19_State[17]);
      #endif
      break;

       case 67:  //Taster 18 
      if (Knx.read(67))    {Taster_0_19_Comand[18] = true;  Taster_0_19_State[18] = true; } 
      else                 {Taster_0_19_Comand[18] = true; Taster_0_19_State[18] = false;}
      #ifdef KDEBUG
      Debug.print("Taster_18");
      Debug.println("%d", Taster_0_19_State[18]);
      #endif
      break;

       case 68:  //Taster 19 
      if (Knx.read(68))    {Taster_0_19_Comand[19] = true;  Taster_0_19_State[19] = true; } 
      else                 {Taster_0_19_Comand[19] = true; Taster_0_19_State[19] = false;}
      #ifdef KDEBUG
      Debug.print("Taster_19");
      Debug.println("%d", Taster_0_19_State[19]);
      #endif
      break;

       case 69:  //Taster 20 
      if (Knx.read(69))    {Taster_0_19_Comand[20] = true;  Taster_0_19_State[20] = true; } 
      else                 {Taster_0_19_Comand[20] = true; Taster_0_19_State[20] = false;}
      #ifdef KDEBUG
      Debug.print("Taster_20");
      Debug.println("%d", Taster_0_19_State[20]);
      #endif
      break;



     
     
     default: break;   
  }
};



