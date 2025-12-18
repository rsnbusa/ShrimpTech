#ifndef TYPESi2c_H_
#define TYPESi2c_H_
#include "includes.h"
#include "typedef.h"
#include "driver/i2c_types.h"

#define FRAM16K

FramI2C::FramI2C(void)
{
	_framInitialised = false;
	intframWords=0;
	addressBytes=0;
	prodId=0;
	manufID=0;
	density=0;
	idev_handle=NULL;
}


bool FramI2C::begin(int sda, int scl, SemaphoreHandle_t *framSem)
{
// printf("I2C fram begin sda %d scl %d Addres %x\n",sda,scl,ESP_SLAVE_ADDR);
	int i2c_master_port = I2C_NUMBER;
	i2c_master_bus_config_t i2c_mst_config;
	bzero(&i2c_mst_config,sizeof(i2c_mst_config));

	i2c_mst_config.i2c_port = I2C_NUMBER;
	i2c_mst_config.sda_io_num =(gpio_num_t)sda;
	i2c_mst_config.scl_io_num =(gpio_num_t)scl;
	i2c_mst_config.clk_source = I2C_CLK_SRC_DEFAULT;
	i2c_mst_config.glitch_ignore_cnt = 7;
	i2c_mst_config.flags= {
			.enable_internal_pullup = true,
			.allow_pd=false
		};


	i2c_master_bus_handle_t bus_handle;
	ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));
	i2c_device_config_t dev_cfg;
	bzero(&dev_cfg,sizeof(dev_cfg));
		dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
		// .device_address = 0x50,
		dev_cfg.device_address = I2C_DEVICE_ADDRESS_NOT_USED;
		dev_cfg.scl_speed_hz = 100000;


	i2c_master_dev_handle_t dev_handle;
	ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));
	idev_handle=dev_handle;

		intframWords=2048;
		*framSem= xSemaphoreCreateBinary();
		if(*framSem)
			xSemaphoreGive(*framSem);  //SUPER important else its born locked
		else
			printf("Cant allocate Fram Sem\n");
	 	return true;
}


int FramI2C::i2c_master_read_slave( uint16_t address, uint8_t *data_rd, size_t size)
{
	uint8_t llow, hhigh;

	llow=address & 0xff;
	hhigh=(address & 0x700)>>8;

    if (size == 0) 
        return ESP_OK;
    
	uint8_t address_w=(ESP_SLAVE_ADDRF+hhigh)<<1 | WRITE_BIT;	//according to manual MB85RC16 page 9 random read,
	uint8_t address_r=(ESP_SLAVE_ADDRF+hhigh)<<1 | READ_BIT;
	uint8_t * dosbytes=(uint8_t*)malloc(2);
	uint16_t twob=llow*256+address_w;
	i2c_operation_job_t i2c_ops[]= {
		{ .command = I2C_MASTER_CMD_START },																						// 1 --> Start
		{ .command = I2C_MASTER_CMD_WRITE, .write = { .ack_check = true, .data = (uint8_t *) &twob, .total_bytes = 2 } },		// 2 --> i2c Address and High bits of read address and write bit
		{ .command = I2C_MASTER_CMD_START },																						// 4 --> restart
		{ .command = I2C_MASTER_CMD_WRITE, .write = { .ack_check = true, .data = (uint8_t *) &address_r, .total_bytes = 1 } },		// 5 --> i2c address and READ bit
		{ .command = I2C_MASTER_CMD_READ, .read = { .ack_value = I2C_ACK_VAL, .data =(uint8_t*) data_rd, .total_bytes = size-1 } },			// 6 --> all but 1 byte to read
		{ .command = I2C_MASTER_CMD_READ, .read = { .ack_value = I2C_NACK_VAL, .data = (uint8_t*)(data_rd+size), .total_bytes = 1 } },			// 7 --> last byte to read
		{ .command = I2C_MASTER_CMD_STOP },																							// 8 --> Stop
	};

	esp_err_t ret=i2c_master_execute_defined_operations(idev_handle, i2c_ops, sizeof(i2c_ops) / sizeof(i2c_operation_job_t), -1);
    return ret;
}

 esp_err_t FramI2C::i2c_master_write_slave(uint16_t address, uint8_t *data_wr, size_t size)
{
	uint8_t llow, hhigh;

	llow=address & 0xff;
	hhigh=(address & 0x700)>>8;

    if (size == 0) 
        return ESP_OK;

	uint8_t address_w=(ESP_SLAVE_ADDRF+hhigh)<<1 | WRITE_BIT;

	uint8_t *allbytes=(uint8_t*)malloc(size+2);
	memcpy(allbytes,&address_w,1);
	memcpy(allbytes+1,&llow,1);
	memcpy(allbytes+2,data_wr,size);
	size+=2;
	i2c_operation_job_t i2c_ops[]= {
		{ .command = I2C_MASTER_CMD_START },																						// start
		{ .command = I2C_MASTER_CMD_WRITE, .write = { .ack_check = true, .data = (uint8_t *) allbytes, .total_bytes = size } },		// address and high bits of address
		{ .command = I2C_MASTER_CMD_STOP },																							// stop
	};
	esp_err_t ret=i2c_master_execute_defined_operations(idev_handle, i2c_ops, sizeof(i2c_ops) / sizeof(i2c_operation_job_t), -1);

	// esp_rom_printf("i2c write add %x -%d size %d addH %x addl %x\n",address,address,size, ((ESP_SLAVE_ADDRF+hhigh) << 1),llow);

	free (allbytes);
    return ret;
}

// #endif
int FramI2C::writeMany (uint16_t framAddr, uint8_t *valores,uint16_t son)
{
	return (i2c_master_write_slave(framAddr,valores,son));
}

int FramI2C::readMany (uint16_t framAddr, uint8_t *valores, uint16_t son)
{
	return (i2c_master_read_slave(framAddr,valores,son));
}

void FramI2C::getDeviceID(uint16_t *manufacturerID, uint16_t *productID,uint8_t *density)
{
	uint8_t data[3];

	// i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    // i2c_master_start(cmd);
    // i2c_master_write_byte(cmd, 0xF8, ACK_CHECK_EN);
    // i2c_master_write_byte(cmd,(ESP_SLAVE_ADDRF << 1) | WRITE_BIT, ACK_CHECK_EN);
    // i2c_master_start(cmd);
    // i2c_master_write_byte(cmd, 0xF9, ACK_CHECK_EN);
    // i2c_master_read_byte(cmd, &data[0], (i2c_ack_type_t)ACK_VAL);
    // i2c_master_read_byte(cmd, &data[1], (i2c_ack_type_t)ACK_VAL);
    // i2c_master_read_byte(cmd, &data[2], (i2c_ack_type_t)NACK_VAL);
    // i2c_master_stop(cmd);
    // esp_err_t ret = i2c_master_cmd_begin(I2C_NUMBER, cmd, pdMS_TO_TICKS(1000));
    // i2c_cmd_link_delete(cmd);
	// Shift values to separate manuf and prod IDs
	// See p.10 of http://www.fujitsu.com/downloads/MICRO/fsa/pdf/products/memory/fram/MB85RC256V-DS501-00017-3v0-E.pdf
	*manufacturerID=(data[0]<<8)+(data[1]&0xF0)>>4;//manufacturer plus density (last 4 bits)
	*productID=((data[1]&0xf)<<8)+data[2];
	// printf("D1 %x  D2 %x\n",data[1],data[2]);
}

//  Derived Commands that use upper basic commands 

int FramI2C::format( uint8_t *lbuffer,uint32_t len,bool all)
{
	uint32_t add=0;	//from the beginning and ALL space available
	int count=intframWords,ret;
	if (len>count)
		len=count;
printf("To format size %d\n",intframWords);
	uint8_t *buffer=(uint8_t*)malloc(len);
	if(!buffer)
	{
		ESP_LOGI(MESH_TAG,"Failed format buf");
		return -1;
	}
	bzero(buffer,len);
	int son=0;
	while (count>0)
	{
		if(lbuffer!=NULL)
				memcpy(buffer,lbuffer,len); //Copy whatever was passed
			else
				memset(buffer,0,len);  //Should be done only once

		if (count>len)
		{
			son+=len;
			printf("Format add %d len %d count %d\n",add,len,count);
			ret=writeMany(add,buffer,len);
			if (ret!=0)
				return ret;
		}
		else
		{
			son+=count;
			printf("FinalFormat add %d len %d\n",add,count);
			ret=writeMany(add,buffer,count);
			if (ret!=0)
				return ret;
		}
		count-=len;
		add+=len;
	}

//	verify
/*
	uint32_t monton=0,total;
	if(all)
	{
		count=intframWords;
		add=0;
		uint8_t *lbuffer=buffer; //copy it
		while(count>0)
		{
			buffer=lbuffer;
			memset(buffer,0,len);

			if(count>len)
				monton=len;
			else
				monton=count;
			ret=readMany(add,buffer,monton);
			if(ret!=0)
			{
				ESP_LOGI(MESH_TAG,"Verify failed HW");
				return ret;
			}
//compare
			total=0;
			for (int a=0;a<monton;a++)
			total+= *(buffer++);
			if (total!=0)
			{
				ESP_LOGI(MESH_TAG,"Verify Logic failed Where %d Total %d",add,total);
				return -1;
			}

			add+=monton;
			count-=monton;
		}
		buffer=lbuffer;			//for free
	}
	*/
	ESP_LOGI(MESH_TAG,"Fram formatted and verified %d",intframWords);
	free(buffer);
	return ESP_OK;
}

// Blower Data Management

int FramI2C::write_bytes(uint32_t add,uint8_t*  desde,uint32_t cuantos)
{
	int ret;	//OB
	ret=writeMany(add,desde,cuantos);	
	return ret;
}

int FramI2C::read_bytes(uint32_t add,uint8_t*  donde,uint32_t cuantos)
{
	int ret;
	ret=readMany(add,donde,cuantos);
	return ret;
}

int FramI2C::write_Blower(uint8_t *mBlower,uint16_t len)
{
	int ret;

	uint32_t badd=0;
	ret=write_bytes(badd,mBlower,len);
	if(ret!=ESP_OK)
		ESP_LOGE(MESH_TAG,"I2C write Blower err %x",ret);
	// esp_rom_printf("Fram write add %x pointer %p len %d\n",badd,mBlower,len);
	return ret;
}


int FramI2C::read_Blower(uint8_t*  address,uint16_t len)
{
	int ret;
	uint32_t badd=0;

	ret=read_bytes(badd,address,len);

	return ret;
}
#endif