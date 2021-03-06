#include "I2C.h"
#include "delay.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"


void Delay(u32 count)//用于产生400KHzIIC信号所需要的延时
{
	while (count--);
}

/**************************实现函数********************************************
*函数原型:		void IIC_Init(void)
*功　　能:		初始化I2C对应的接口引脚。
*******************************************************************************/
void IIC_Init(void)
{			
	GPIO_InitTypeDef GPIO_InitStructure;
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);			     
 	//配置PB10 PB11 为开漏输出  刷新频率为50Mhz
 	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;       
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	//应用配置到GPIOB 
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	SDA_OUT();     // SDA线输出
	// 将两条线拉高
	IIC_SDA=1;	  
	IIC_SCL=1;
}

/**************************实现函数********************************************
*函数原型:		void IIC_Start(void)
*功　　能:		产生IIC起始信号
*******************************************************************************/
void IIC_Start(void)
{
	SDA_OUT();     //SDA线输出
	//初始化两条线操作，确保都为高电平
	IIC_SDA=1;  
	IIC_SCL=1;
	
	Delay(5);// 延时，保证在SDA和SCL电平由低拉高这个过程中，电平不受到之后的电平操作影响
 	IIC_SDA=0;// 开始信号： SCL为高， SDA由高变低
	
	Delay(5);// 延时，保证在SDA电平开始变化到变化完成过程中，SCL线始终为低
	IIC_SCL=0;// 钳住I2C总线，准备发送或接收数据 
}

/**************************实现函数********************************************
*函数原型:		void IIC_Stop(void)
*功　　能:	    //产生IIC停止信号
*******************************************************************************/	  
void IIC_Stop(void)
{
	SDA_OUT();// SDA线输出
	IIC_SCL=0;// 确保此时SCL线为低，留出时间，给SDA线进行电平转换
	IIC_SDA=0;// 此时，SDA线置低，准备发送停止信号
 	
	Delay(5);// 延迟确保停止信号发送前两条线都是低电平
	IIC_SCL=1;// SCL拉高
	IIC_SDA=1;// 停止信号： SCL线为高， SDA线由高置低
	
	Delay(5);// 延时，确保从设备检测到停止信号					   	
}

/**************************实现函数********************************************
*函数原型:		u8 IIC_Wait_Ack(void)
*功　　能:	    等待应答信号到来 
//返回值：1，接收应答失败
//        0，接收应答成功
*******************************************************************************/
u8 IIC_Wait_Ack(void)
{
	u8 ucErrTime=0;// 计时变量
	SDA_IN();// 等待应答信号时，主设备属于接收方，所以设置SDA线为输入，即将SDA线控制权交由从设备  
	IIC_SDA=1;// 根据IIC协议拉高SDA线，此时从设备若没有拉低SDA线即非应答
	Delay(5);// 延时，留给从设备拉高SDA线的时间
	while(READ_SDA)// 不断的读SDA线的值，当SDA线值为0，退出循环，即表示收到应答信号
	{
		ucErrTime++;// 读到的SDA线值不为0，进入循环，计时变量加1
		if(ucErrTime>50)// 如果计时变量大与50，SDA线还未为0，即没有收到应答信号，从而发送一个停止信号
		{
			IIC_Stop();// 发送停止信号
			return 1;// 停止信号发送，即表示结束此次通信
		}
		Delay(5);// 每次准备重复循环时，延时，留给SDA线电平转换时间
	}
	// 走出循环，表示已接收到应答信号
	IIC_SCL=1;// 根据IIC协议，把SCL线拉高
	Delay(5); // 延时，保证整个SCL线高电平期间可以收集到SDA信号
	IIC_SCL=0;// SCL线置0，准备下次发送数据
	return 0; 
} 

/**************************实现函数********************************************
*函数原型:		void IIC_Ack(void)
*功　　能:	    产生ACK应答
*******************************************************************************/
void IIC_Ack(void)
{
	IIC_SCL=0;// 先将SCL线拉低，留出时间，让SDA线得以进行电平变化
	SDA_OUT();// 应答信号由主设备发出，需要SDA线控制权
	IIC_SDA=0;// 将SDA线置低，表示应答信号
	
	Delay(5);// 延时，确保SDA线电平为低时，SCL线再拉高 
	IIC_SCL=1;// SCL线拉高，即协议中的数据有效性原则
	Delay(5);// 延时，确保SCL线拉高一段时间，确保从设备检测到应答信号
	IIC_SCL=0;// 将SCL线置低，准备接收下一次数据
}
	
/**************************实现函数********************************************
*函数原型:		void IIC_NAck(void)
*功　　能:	    产生NACK应答
*******************************************************************************/	    
void IIC_NAck(void)
{
	IIC_SCL=0;// 先将SCL线拉低，留出时间，让SDA线得以进行电平变化
	SDA_OUT();// 应答信号由主设备发出，需要SDA线控制权
	IIC_SDA=1;// 将SDA线置高，表示非应答信号
	
	Delay(5);// 延时，确保SDA线电平为高时，SCL线再拉高 
	IIC_SCL=1;// SCL线拉高，即协议中的数据有效性原则
	Delay(5);// 延时，确保SCL线拉高一段时间，确保从设备检测到非应答信号
	IIC_SCL=0;// 将SCL线置低，准备进行下一次数据通信
}					 				     

/**************************实现函数********************************************
*函数原型:		void IIC_Send_Byte(u8 txd)
*功　　能:	    IIC发送一个字节
*******************************************************************************/		  
void IIC_Send_Byte(u8 txd)
{                        
    u8 t;// 数据计数变量
		SDA_OUT();// 由于发送一个字节，主设备为发送方，需要SDA线的控制权 	    
    IIC_SCL=0;//拉低时钟开始数据传输，留给SDA线电平转换时间
    for(t=0;t<8;t++){// 表示发送8位数据              
        IIC_SDA=(txd&0x80)>>7;// 将要发送的数据与上0x80，相当于只保留要发送的数据的第7位（最高位）。 再右移7位，相当于把要发送的数据的第7位（最高位），赋给SDA线。 由此SDA线的高低电平即表示由高到低每一个数据位。
        txd<<=1;// 将要发送的数据左移一位，相当于把要发送的数据的第6位（次高位）变成了第7位（最高位）。由此循环8次，那么8位要发送数据就依次由SDA线发送出去了。
			
				Delay(2);// 延时，确保SDA线的电平已经变化完成  
				IIC_SCL=1;// SCL线拉高，即协议中的数据有效性原则
				Delay(5);// 延时，确保SCL线拉高一段时间，确保从设备检测到此时的数据位
				IIC_SCL=0;// 将SCL线置低，准备进行下一次数据位的发送
				Delay(3);// 延时，留给SDA线电平转换时间
    }	 
} 	 
   
/**************************实现函数********************************************
*函数原型:		u8 IIC_Read_Byte(unsigned char ack)
*功　　能:	    //读1个字节
*ack=1时，发送ACK， ack=0，发送NACK 
*******************************************************************************/  
u8 IIC_Read_Byte(unsigned char ack)
{
	unsigned char i,receive=0;// receive：收到的数据变量
	SDA_IN();// 由于读数据，此时主设备作为接收方，SDA设置为输入，将SDA线控制权释放
  for( i=0;i<8;i++ ){// 表示读取8位数据 
      IIC_SCL=0;// 将SCL线拉低，留给SDA线电平变化时间
			Delay(5);// 延时，确保SDA线电平变化完成
			IIC_SCL=1;// SCL线拉高，即协议中的数据有效性原则
      receive<<=1;// 将收到的数据左移1位，即高位的数据向高位逐次向上推，直到第8次，即数据接收完成
      if(READ_SDA)receive++;// 如果此时SDA线为1，即当前数据位为1，将这个数据位加到收到的数据变量上， 如果为0，则不需要任何操作，继续循环   
			Delay(5);// 此延时作用尚未理解
  }				
	// 是否发送应答信号，即是否继续接收数据
  if (ack)
      IIC_Ack(); //发送ACK 
  else
      IIC_NAck();//发送nACK  
			return receive;
}

/**************************实现函数********************************************
*函数原型:		u8 IICreadBytes(u8 dev, u8 reg, u8 length, u8 *data)
*功　　能:	    读取指定设备 指定寄存器的 length个字节值
输入	dev  目标设备地址
		reg	  寄存器地址
		length 要读的字节数
		*data  读出的数据将要存放的指针
返回   读出来的字节数量
*******************************************************************************/ 
u8 IICreadBytes(u8 dev, u8 reg, u8 length, u8 *data){
    u8 count = 0;// 读取的第几个数据的个数变量
	
	IIC_Start();// 通信开始，发送起始信号
	IIC_Send_Byte(dev<<1);// 由于地址位为7位，且发送的为写命令（读写位为0），所以左移1位地址后，无需操作，即发送了从设备地址和读写位的写命令
	IIC_Wait_Ack();// 等待应答信号
	//u8 ack = IIC_Wait_Ack();
	//if ( ack != 0 ) return -1;// 如果为非应答信号直接退出循环
	IIC_Send_Byte(reg);// 发送寄存器的地址
  IIC_Wait_Ack();// 等待应答信号
	//ack = IIC_Wait_Ack();
	//if ( ack != 0 ) return -1;// 如果为非应答信号直接退出循环
	IIC_Start();// 再次发送起始信号
	IIC_Send_Byte((dev<<1)+1);// 发送从设备地址和1位读写位（由于需要读数据，即读写位为1），进入接收模式	
	IIC_Wait_Ack();// 等待应答信号
	//ack = IIC_Wait_Ack();
	//if ( ack != 0 ) return -1;// 如果为非应答信号直接退出循环
	
  for(count=0;count<length;count++){
		 
		 if(count!=length-1)data[count]=IIC_Read_Byte(1);// 如果不是最后一个数据，执行一个带ACK的读数据操作
		 	else  data[count]=IIC_Read_Byte(0);// 最后读取一个带NACK的读数据操作
	}
  IIC_Stop();// 发送一个停止信号
  return count;
}

/**************************实现函数********************************************
*函数原型:		u8 IICwriteBytes(u8 dev, u8 reg, u8 length, u8* data)
*功　　能:	    将多个字节写入指定设备 指定寄存器
输入	dev  目标设备地址
		reg	  寄存器地址
		length 要写的字节数
		*data  将要写的数据的首地址
返回   返回是否成功
*******************************************************************************/ 
u8 IICwriteBytes(u8 dev, u8 reg, u8 length, u8* data){
  
 	u8 count = 0;// 发送的第几个数据的个数变量
	IIC_Start();// 发送一个起始信号
	IIC_Send_Byte(dev<<1);// 由于地址位为7位，且发送的为写命令（读写位为0），所以左移1位地址后，无需操作，即发送了从设备地址和读写位的写命令
	IIC_Wait_Ack();// 等待应答信号
	//ack = IIC_Wait_Ack();
	//if ( ack != 0 ) return -1;// 如果为非应答信号直接退出循环
	IIC_Send_Byte(reg);// 发送寄存器地址
	IIC_Wait_Ack();// 等待应答信号	 
//ack = IIC_Wait_Ack();
	//if ( ack != 0 ) return -1;// 如果为非应答信号直接退出循环	
	
	for(count=0;count<length;count++){
		IIC_Send_Byte(data[count]); 
		IIC_Wait_Ack();// 等待应答信号
		//ack = IIC_Wait_Ack();
	  //if ( ack != 0 ) return -1;// 如果为非应答信号直接退出循环
  }
	IIC_Stop();// 发送一个停止信号

  return 1;
	
}
