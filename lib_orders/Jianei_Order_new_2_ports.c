#include "Jianei_Order.h"

extern struct orderStruct Jianei_Order;

#define WEBMASTER_DATA_ORDERNUM_INDEX      21
#define WEBMASTER_DATA_ORDERNUM_LENGTH     17
#define WEBMASTER_DATA_UUID_INDEX          38
#define WEBMASTER_DATA_UUID_LENGTH         30
#define WEBMASTER_DATA_PORTA_INDEX         68
#define WEBMASTER_DATA_PORTZ_INDEX         71

#define EID_VAILD_DATA_INDEX			   9	 
#define EID_VAILD_DATA_LENGTH              16

void Jianei_Order_new_2_ports(unsigned char *src, unsigned char data_source, int sockfd)
{
	uint8_t ofs = 0, jkb_idex = 0, eidtype = 32;

	if(data_source == DATA_SOURCE_MOBILE || data_source == DATA_SOURCE_NET)
	{
		if(0xa2 != src[21] && 0x9a != src[21]){
			/* 清除架内数据结构体,并记录下此次施工的工单号 */
			memset((void *)&Jianei_Order, 0x0, sizeof(Jianei_Order));
			memcpy(&Jianei_Order.ord_number[0], &src[WEBMASTER_DATA_ORDERNUM_INDEX], WEBMASTER_DATA_ORDERNUM_LENGTH);

			/* 如果手机发来数据中的uuid跟本机架的不一样，就不做该工单，并返回个错误码0x01给手机 */
			if(!myStringcmp(&boardcfg.box_id[0],&src[WEBMASTER_DATA_UUID_INDEX], WEBMASTER_DATA_UUID_LENGTH)){
				//orderoperations_failure_and_reply_to_mobile(CMD_NEW_2_PORTS, 0x01);
				printf("uuid not match ..... \n");
				return ;
			}
			
			if(data_source == DATA_SOURCE_NET){
				Jianei_Order.ord_type = ORDER_NET;
				Jianei_Order.ord_fd = sockfd;
			}

			/* 打开工单灯。表征机架正在执行工单 */
			ioctl(alarmfd, 3, 0);

			/* 准备数据，(如果同框的端口，就准备一帧 . 手机端发来的数据中，下标38位跟41位分别代表两个端口) */ 
			memcpy(&Jianei_Order.eid_port1[0], &src[WEBMASTER_DATA_PORTA_INDEX], 3);
			memcpy(&Jianei_Order.eid_port2[0], &src[WEBMASTER_DATA_PORTZ_INDEX], 3);

			if(Jianei_Order.eid_port1[0] == Jianei_Order.eid_port2[0]){
				/* 要新建的两个端口在同一个框中，只需要发给一个接口板即可*/
				Jianei_Order.zkb_rep0d[0] = 0x7e;
				Jianei_Order.zkb_rep0d[1] = 0x00;
				Jianei_Order.zkb_rep0d[2] = 0x00;
				Jianei_Order.zkb_rep0d[3] = 0x0d;
				Jianei_Order.zkb_rep0d[4] = CMD_NEW_2_PORTS;
				Jianei_Order.zkb_rep0d[5] = 0x41;
				ofs = 6;
				memcpy(&Jianei_Order.zkb_rep0d[ofs], &src[WEBMASTER_DATA_PORTA_INDEX], 3);
				ofs += 4;
				memcpy(&Jianei_Order.zkb_rep0d[ofs], &src[WEBMASTER_DATA_PORTZ_INDEX], 3);
				ofs += 4;
				ofs += 32;

				Jianei_Order.zkb_rep0d[2] = ofs;
				Jianei_Order.zkb_rep0d[ofs] = calccrc8(&Jianei_Order.zkb_rep0d[0], ofs);
				Jianei_Order.zkb_rep0d[ofs+1] = 0x5a;

				jkb_idex = Jianei_Order.eid_port1[0];
				memset(&uart0.txBuf[jkb_idex-1][0],0,TX_BUFFER_SIZE);
				memcpy(&uart0.txBuf[jkb_idex-1][0],&Jianei_Order.zkb_rep0d[0], ofs+2);	
				
			}else{
				/* 要新建的两个端口，不在同一个框，得发向两个接口板上*/	
				Jianei_Order.zkb_rep0d[0] = 0x7e;
				Jianei_Order.zkb_rep0d[1] = 0x00;
				Jianei_Order.zkb_rep0d[2] = 0x00;
				Jianei_Order.zkb_rep0d[3] = 0x0d;
				Jianei_Order.zkb_rep0d[4] = CMD_NEW_2_PORTS;
				Jianei_Order.zkb_rep0d[5] = 0x41;
				ofs = 6;
				memcpy(&Jianei_Order.zkb_rep0d[ofs], &src[WEBMASTER_DATA_PORTA_INDEX], 3);
				ofs += 4;
				memset(&Jianei_Order.zkb_rep0d[ofs], 0x0, 3);
				ofs += 4;
				ofs += 32;

				Jianei_Order.zkb_rep0d[2] = ofs;
				Jianei_Order.zkb_rep0d[ofs] = calccrc8(&Jianei_Order.zkb_rep0d[0], ofs);
				Jianei_Order.zkb_rep0d[ofs+1] = 0x5a;

				jkb_idex = Jianei_Order.eid_port1[0];
				memset(&uart0.txBuf[jkb_idex-1][0],0,TX_BUFFER_SIZE);
				memcpy(&uart0.txBuf[jkb_idex-1][0],&Jianei_Order.zkb_rep0d[0], ofs+2);			

				ofs = 6;
				memcpy(&Jianei_Order.zkb_rep0d[ofs], &src[WEBMASTER_DATA_PORTZ_INDEX], 3);
				ofs += 4;
				//memset(&Jianei_Order.zkb_rep0d[ofs], 0x0, 3);
				ofs += 4;
				ofs += 32;	

				Jianei_Order.zkb_rep0d[2] = ofs;
				Jianei_Order.zkb_rep0d[ofs] = calccrc8(&Jianei_Order.zkb_rep0d[0], ofs);
				jkb_idex = Jianei_Order.eid_port2[0];
				memset(&uart0.txBuf[jkb_idex-1][0],0,TX_BUFFER_SIZE);
				memcpy(&uart0.txBuf[jkb_idex-1][0],&Jianei_Order.zkb_rep0d[0], ofs+2);					

			}

            /* 将准备好的数据，发送给接口板,如果发送失败，返回错误码0x03*/			
			if(!module_order_tasks_to_sim3u1xx_board()){
				//orderoperations_failure_and_reply_to_mobile(CMD_NEW_2_PORTS, 0x03);
				printf("%s . %d \n", __func__, __LINE__);
				return ;
			}else{
				printf("%s . %d \n", __func__, __LINE__);
			}

		}else{

			printf("now cancle order ....\n");

			/* 手机端取消了该工单 */
			zkb_issue_cancle_command_to_jkb(src, src[21]);

            /* 将准备好的数据，发送给接口板*/			
			if(!module_order_tasks_to_sim3u1xx_board()){
			//	orderoperations_failure_and_reply_to_mobile(CMD_NEW_2_PORTS, 0x03);
				return ;
			}
		}
		

	}else if(data_source == DATA_SOURCE_UART)
	{
		/* 新建中的端口，有一端发来了标签插入的信息 */	
		if(0x03 == src[5])
		{
			/* 判断是哪个端口发来的，是eid_port1 还是eid_port2 */
            if(myStringcmp(&Jianei_Order.eid_port1[0], &src[7], 3)){
		   		if(isArrayEmpty(&Jianei_Order.eid_port1[4], eidtype)){
					if(isArrayEmpty(&Jianei_Order.eid_port2[4], eidtype)){
						memcpy(&Jianei_Order.eid_port1[4], &src[10], eidtype);
						Jianei_Order.zkb_rep0d[0] = 0x7e;
						Jianei_Order.zkb_rep0d[1] = 0x00;
						Jianei_Order.zkb_rep0d[2] = 0x00;
						Jianei_Order.zkb_rep0d[3] = 0x0d;
						Jianei_Order.zkb_rep0d[4] = CMD_NEW_2_PORTS;
						Jianei_Order.zkb_rep0d[5] = 0x41;
						ofs = 6;
						memcpy(&Jianei_Order.zkb_rep0d[ofs], &Jianei_Order.eid_port2[0], 4);
						ofs += 4;
						memset(&Jianei_Order.zkb_rep0d[ofs], 0x0, 4);
						ofs += 4;
						memcpy(&Jianei_Order.zkb_rep0d[ofs], &src[10], eidtype);
						ofs += 32;
						Jianei_Order.zkb_rep0d[2] = ofs;
						Jianei_Order.zkb_rep0d[ofs] = calccrc8(&Jianei_Order.zkb_rep0d[0], ofs);
						Jianei_Order.zkb_rep0d[ofs+1] = 0x5a;

						jkb_idex = Jianei_Order.eid_port2[0];
						memset(&uart0.txBuf[jkb_idex-1][0],0,TX_BUFFER_SIZE);
						memcpy(&uart0.txBuf[jkb_idex-1][0],&Jianei_Order.zkb_rep0d[0], ofs+2);			
					}else{
						if(myStringcmp(&Jianei_Order.eid_port2[8], &src[14], 16)){
							memcpy(&Jianei_Order.eid_port1[4], &src[10], eidtype);
						}else{
							Jianei_Order.zkb_rep0d[0] = 0x7e;
							Jianei_Order.zkb_rep0d[1] = 0x00;
							Jianei_Order.zkb_rep0d[2] = 0x00;
							Jianei_Order.zkb_rep0d[3] = 0x0d;
							Jianei_Order.zkb_rep0d[4] = CMD_NEW_2_PORTS;
							Jianei_Order.zkb_rep0d[5] = 0x41;
							ofs = 6;
							memcpy(&Jianei_Order.zkb_rep0d[ofs], &Jianei_Order.eid_port1[0], 4);
							ofs += 4;
							memset(&Jianei_Order.zkb_rep0d[ofs], 0x0, 4);
							ofs += 4;
							memcpy(&Jianei_Order.zkb_rep0d[ofs], &Jianei_Order.eid_port2[4], eidtype);
							ofs += 32;
							Jianei_Order.zkb_rep0d[2] = ofs;
							Jianei_Order.zkb_rep0d[ofs] = calccrc8(&Jianei_Order.zkb_rep0d[0], ofs);
							Jianei_Order.zkb_rep0d[ofs+1] = 0x5a;
	
							jkb_idex = Jianei_Order.eid_port1[0];
							memset(&uart0.txBuf[jkb_idex-1][0],0,TX_BUFFER_SIZE);
							memcpy(&uart0.txBuf[jkb_idex-1][0],&Jianei_Order.zkb_rep0d[0], ofs+2);							
						}
					}
				}	
		   
		    }else if(myStringcmp(&Jianei_Order.eid_port2[0], &src[7], 3)){
			   	if(isArrayEmpty(&Jianei_Order.eid_port2[4], eidtype)){
					if(isArrayEmpty(&Jianei_Order.eid_port1[4], eidtype)){
						memcpy(&Jianei_Order.eid_port2[4], &src[10], eidtype);
						Jianei_Order.zkb_rep0d[0] = 0x7e;
						Jianei_Order.zkb_rep0d[1] = 0x00;
						Jianei_Order.zkb_rep0d[2] = 0x00;
						Jianei_Order.zkb_rep0d[3] = 0x0d;
						Jianei_Order.zkb_rep0d[4] = CMD_NEW_2_PORTS;
						Jianei_Order.zkb_rep0d[5] = 0x41;
						ofs = 6;
						memcpy(&Jianei_Order.zkb_rep0d[ofs], &Jianei_Order.eid_port1[0], 4);
						ofs += 4;
						memset(&Jianei_Order.zkb_rep0d[ofs], 0x0, 4);
						ofs += 4;
						memcpy(&Jianei_Order.zkb_rep0d[ofs], &src[10], eidtype);
						ofs += 32;
						Jianei_Order.zkb_rep0d[2] = ofs;
						Jianei_Order.zkb_rep0d[ofs] = calccrc8(&Jianei_Order.zkb_rep0d[0], ofs);
						Jianei_Order.zkb_rep0d[ofs+1] = 0x5a;

						jkb_idex = Jianei_Order.eid_port1[0];
						memset(&uart0.txBuf[jkb_idex-1][0],0,TX_BUFFER_SIZE);
						memcpy(&uart0.txBuf[jkb_idex-1][0],&Jianei_Order.zkb_rep0d[0], ofs+2);			
					}else{
						if(myStringcmp(&Jianei_Order.eid_port1[8], &src[14], 16)){
							memcpy(&Jianei_Order.eid_port2[4], &src[10], eidtype);
						}else{
							Jianei_Order.zkb_rep0d[0] = 0x7e;
							Jianei_Order.zkb_rep0d[1] = 0x00;
							Jianei_Order.zkb_rep0d[2] = 0x00;
							Jianei_Order.zkb_rep0d[3] = 0x0d;
							Jianei_Order.zkb_rep0d[4] = CMD_NEW_2_PORTS;
							Jianei_Order.zkb_rep0d[5] = 0x41;
							ofs = 6;
							memcpy(&Jianei_Order.zkb_rep0d[ofs], &Jianei_Order.eid_port2[0], 4);
							ofs += 4;
							memset(&Jianei_Order.zkb_rep0d[ofs], 0x0, 4);
							ofs += 4;
							memcpy(&Jianei_Order.zkb_rep0d[ofs], &Jianei_Order.eid_port1[4], eidtype);
							ofs += 32;
							Jianei_Order.zkb_rep0d[2] = ofs;
							Jianei_Order.zkb_rep0d[ofs] = calccrc8(&Jianei_Order.zkb_rep0d[0], ofs);
							Jianei_Order.zkb_rep0d[ofs+1] = 0x5a;
	
							jkb_idex = Jianei_Order.eid_port2[0];
							memset(&uart0.txBuf[jkb_idex-1][0],0,TX_BUFFER_SIZE);
							memcpy(&uart0.txBuf[jkb_idex-1][0],&Jianei_Order.zkb_rep0d[0], ofs+2);							
						}
					}
				}		   
		    }

			/* 如果两端都插入了标签，而且是一致的，那就给这两端口发自动确认命令*/
			if(myStringcmp(&Jianei_Order.eid_port1[EID_VAILD_DATA_INDEX],&Jianei_Order.eid_port2[EID_VAILD_DATA_INDEX], EID_VAILD_DATA_LENGTH)
					&& !isArrayEmpty(&Jianei_Order.eid_port1[4], eidtype))
			{
				if(Jianei_Order.eid_port1[0] == Jianei_Order.eid_port2[0]){
					/* 同框的数据*/
					Jianei_Order.zkb_rep0d[0] = 0x7e;
					Jianei_Order.zkb_rep0d[1] = 0x00;
					Jianei_Order.zkb_rep0d[2] = 0x00;
					Jianei_Order.zkb_rep0d[3] = 0x0d;
					Jianei_Order.zkb_rep0d[4] = 0x0a;
					ofs = 6;
					memcpy(&Jianei_Order.zkb_rep0d[ofs], &Jianei_Order.eid_port1[0], 4);
					ofs += 4;
					memcpy(&Jianei_Order.zkb_rep0d[ofs], &Jianei_Order.eid_port2[0], 4);
					ofs += 4;
					ofs += 32;

					Jianei_Order.zkb_rep0d[2] = ofs;
					Jianei_Order.zkb_rep0d[ofs] = calccrc8(&Jianei_Order.zkb_rep0d[0], ofs);
					Jianei_Order.zkb_rep0d[ofs+1] = 0x5a;

					jkb_idex = Jianei_Order.eid_port1[0];
					memset(&uart0.txBuf[jkb_idex-1][0],0,TX_BUFFER_SIZE);
					memcpy(&uart0.txBuf[jkb_idex-1][0],&Jianei_Order.zkb_rep0d[0], ofs+2);	
				}else{
					/* 不同框的数据*/
					Jianei_Order.zkb_rep0d[0] = 0x7e;
					Jianei_Order.zkb_rep0d[1] = 0x00;
					Jianei_Order.zkb_rep0d[2] = 0x00;
					Jianei_Order.zkb_rep0d[3] = 0x0d;
					Jianei_Order.zkb_rep0d[4] = 0x0a;
					/* 第一个框的数据*/
					ofs = 6;
					memcpy(&Jianei_Order.zkb_rep0d[ofs], &Jianei_Order.eid_port1[0], 4);
					ofs += 4;
					memset(&Jianei_Order.zkb_rep0d[ofs], 0x0, 4);
					ofs += 4;
					ofs += 32;

					Jianei_Order.zkb_rep0d[2] = ofs;
					Jianei_Order.zkb_rep0d[ofs] = calccrc8(&Jianei_Order.zkb_rep0d[0], ofs);
					Jianei_Order.zkb_rep0d[ofs+1] = 0x5a;
					jkb_idex = Jianei_Order.eid_port1[0];
					memset(&uart0.txBuf[jkb_idex-1][0],0,TX_BUFFER_SIZE);
					memcpy(&uart0.txBuf[jkb_idex-1][0],&Jianei_Order.zkb_rep0d[0], ofs+2);	

					/* 第二个框的数据 */
					ofs = 6;
					memcpy(&Jianei_Order.zkb_rep0d[ofs], &Jianei_Order.eid_port2[0], 4);
					ofs += 4;
					//memset(&Jianei_Order.zkb_rep0d[ofs], 0x0, 3);
					ofs += 4;
					ofs += 32;	

					Jianei_Order.zkb_rep0d[2] = ofs;
					Jianei_Order.zkb_rep0d[ofs] = calccrc8(&Jianei_Order.zkb_rep0d[0], ofs);
					jkb_idex = Jianei_Order.eid_port2[0];
					memset(&uart0.txBuf[jkb_idex-1][0],0,TX_BUFFER_SIZE);
					memcpy(&uart0.txBuf[jkb_idex-1][0],&Jianei_Order.zkb_rep0d[0], ofs+2);					
				}

			}

			/* 将准备好的数据，发送给接口板*/			
			if(!module_order_tasks_to_sim3u1xx_board()){
				//orderoperations_failure_and_reply_to_mobile(CMD_NEW_2_PORTS, 0x03);
				return ;
			}

		}else if(0x8c == src[5])
		{/* 新建中的端口有拔出动作*/
			if((myStringcmp(&Jianei_Order.eid_port1[0], &src[7], 3) && isArrayEmpty(&Jianei_Order.eid_port2[4], 32))
				|| (myStringcmp(&Jianei_Order.eid_port2[0], &src[7], 3) && isArrayEmpty(&Jianei_Order.eid_port1[4], 32))
			)
			{
				zkb_issue_output_command_to_jkb(NULL, 0x8c);
 	           /* 将准备好的数据，发送给接口板*/			
				if(!module_order_tasks_to_sim3u1xx_board()){
					orderoperations_failure_and_reply_to_mobile(CMD_NEW_2_PORTS, 0x03);
					return ;
				}
			}

		}else if(0x0a == src[5])
		{
			/* 确认完工的端口发来数据*/
			if(myStringcmp(&Jianei_Order.eid_port1[0], &src[7], 3)){
				if(0x00 == src[11]){
					Jianei_Order.eid_port1[40] = 0x0a;
				}else{ 
					Jianei_Order.eid_port1[40] = 0x0e;
				}
			}else if(myStringcmp(&Jianei_Order.eid_port2[0], &src[7], 3)){
				if(0x00 == src[11]){
					Jianei_Order.eid_port2[40] = 0x0a;
				}else{ 
					Jianei_Order.eid_port2[40] = 0x0e;		
				}
			}
			
			if(0x0a == Jianei_Order.eid_port1[40] && 0x0a == Jianei_Order.eid_port2[40]){
				/* 工单完成 */
				orderoperations_success_and_reply_to_mobile(NULL, CMD_NEW_2_PORTS, 0x00);

			}else if(0x0e == Jianei_Order.eid_port1[40] || 0x0e == Jianei_Order.eid_port2[40]){
				/* 工单端口确认失败，将端口恢复成工单执行前状态*/
				Jianei_Order.eid_port1[3] = DYB_PORT_NOT_USED;
				Jianei_Order.eid_port2[3] = DYB_PORT_NOT_USED;
				zkb_issue_cancle_command_to_jkb(NULL, 0x9c);

				if(!module_order_tasks_to_sim3u1xx_board()){
					orderoperations_failure_and_reply_to_mobile(CMD_NEW_2_PORTS, 0x03);
					return ;
				}

				/* 将工单确认失败的结果，告知手机端，以提示工单操作人员 */
				orderoperations_failure_and_reply_to_mobile(CMD_NEW_2_PORTS, 0x07);
			}

		}

	}

}
