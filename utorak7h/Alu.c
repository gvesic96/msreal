
/*
Zadatak utorak 07h treba napraviti drajver koji modeluje ALU za rad sa pozitivnim celobrojnim
vrednostima. Alu se sastoji iz 4 osmobitna registra, regA, regB, regC i regD u koje se smestaju
podaci, registar result za cuvanje podataka i statusni registar carry idnikator prekoracenja
opsega.
izvesti i blokiranje procesa za pun i prazan bafer
zastiti deljeni resurs to jest fifo buffer semaforom da ne bi doslo do hazarda
*/
/*
Obavezno dozvoliti upis u fifo node fajl sa - sudo chmod a+rw /dev/fifo
*/

//KOMANDE CAT I ECHO POKRETATI SA & NA KRAJU DA BI SE KREIRAO PROCES U POZADINI !!!

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/device.h>

#include <linux/semaphore.h>

//needed for blocking
#include <linux/wait.h>
DECLARE_WAIT_QUEUE_HEAD(readQ);
DECLARE_WAIT_QUEUE_HEAD(writeQ);

//struktura semafor potrebna za realizaciju semafora
struct semaphore sem;

#define BUFF_SIZE 20

MODULE_LICENSE("Dual BSD/GPL");

dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
static struct cdev *my_cdev;

int regA,regB,regC,regD;
int endRead = 0;
int result, carry;
int read_mode = 1;

int alu_open(struct inode *pinode, struct file *pfile);
int alu_close(struct inode *pinode, struct file *pfile);
ssize_t alu_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset);
ssize_t alu_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset);

struct file_operations my_fops =
{
	.owner = THIS_MODULE,
	.open = alu_open,
	.read = alu_read,
	.write = alu_write,
	.release = alu_close,
};


int alu_open(struct inode *pinode, struct file *pfile) 
{
		printk(KERN_INFO "Succesfully opened alu\n");
		return 0;
}

int alu_close(struct inode *pinode, struct file *pfile) 
{
		printk(KERN_INFO "Succesfully closed alu\n");
		return 0;
}

ssize_t alu_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset) 
{
	int ret;
	char buff[BUFF_SIZE];
	long int len = 0;

	int mask = 1;
	char tmp[BUFF_SIZE];
	int i, j;

	if (endRead){
		endRead = 0;
		return 0;
	}

	/*
	//proces zauzima semafor
	if(down_interruptible(&sem))
		return -ERESTARTSYS;
	while(pos == 0)
	{
		ulazi while petlju, oslobadja semafor kako bi neki drugi proces mogao da upise i
		blokira se u redu cekanja citanja
		up(&sem);
		Ukoliko nista nije upisano proces se blokira u ovoj liniji i smesta u red cekanja
		za citanje readQ
		if(wait_event_interruptible(readQ,(pos>0)))
			return -ERESTARTSYS;
		Nakon budjenja procesa koji cekaju u redu za cigtanje readQ ova funkcija
		nastavlja da se izvrsava odavde. Proces citanja se budi na kraju funkcije za upis,
		gde se poziva funkcija za budjenje procesa citanja wake_up_interruptible(&readQ)

		//ponovo zauzima semafor nakon sto je probudjen iz reda cekanja na citanje
		if(down_interruptible(&sem))
			return -ERESTARTSYS;
		*/

		switch(read_mode)
		{
		case 1: {len = scnprintf(buff, BUFF_SIZE, "%d", result);
			break;
			}
		case 2: {len = scnprintf(buff, BUFF_SIZE, "%d", result);
			break;
			}
		case 3: {
			j=7;
				for(i=0;i<8;i++){
				tmp[i] = ((result & (mask<<j)) >> j) + '0';
				j--;
				}
			len = scnprintf(buff, BUFF_SIZE, tmp);
			break;
			}
		default: printk(KERN_INFO "Reading format constant not set...");
			len = 0;
			break;
		}

		//len = scnprintf(buff, BUFF_SIZE, "%d ", result);
		ret = copy_to_user(buffer, buff, len);

		if(ret)
			return -EFAULT;


		endRead = 1;
		printk(KERN_INFO "Succesfully read\n");//uspesno procitan podatak fifo[0]

		//cat command will keep calling again until len doesnt come to 0 which is EOF


	/*
	up(&sem);//oslobadanje semafora izlazak iz kriticne sekcije
	wake_up_interruptible(&writeQ);//ova linija budi procese koji cekaju u redu za upis
	*/

	return len;
}

ssize_t alu_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset)
{
	char buff[BUFF_SIZE];
	char buff_tmp[7] = {'f','o','r','m','a','t','='};
	char buff_tmp2[3] = {'r','e','g'};
	char buff_tmp3[9] = {'d','e','c','h','e','x','b','i','n'};
	//int value;
	int ret;
	int i;
	int reg1 = 0;
	int reg2 = 0;
	int tmp;

	tmp = read_mode; //setting current value of reading mode


	ret = copy_from_user(buff, buffer, length);//successful execution returns 0
	/*buff je kernel niz gde se smesta buffer je niz u kurisnickom prostoru,
	length je duzina niza koji se kopira*/


	if(ret)
		return -EFAULT;

	buff[length-1] = '\0'; //niz krece od nula a length je ukupan broj clanova niza buffer

	for(i=0;i<7;i++){
		if(buff[i] == buff_tmp[i]){
			ret = 1;
		}else{
			ret = 0; break;
		}
	}

	if(ret == 1){

		for(i=0;i<3;i++){
			if(buff[i+7] == buff_tmp3[i]){
				tmp = 1; ret = 0;}
			else {ret = 1; break;}
		}
		if(ret == 1){
		for(i=0;i<3;i++){
			if(buff[i+7] == buff_tmp3[i+3]){
				tmp = 2; ret = 0;}
			else {ret = 1; break;}
		}
		}//if

		if(ret == 1){
		for(i=0;i<3;i++){
			if(buff[i+7] == buff_tmp3[i+6]){
				tmp = 3; ret = 0;}
			else {ret =1; break;}
		}
		}//if
		if(ret == 0){
			read_mode = tmp;
			switch(read_mode){
				case 1: printk(KERN_INFO "Reading format set to decimal\n"); break;
				case 2: printk(KERN_INFO "Reading format set to hexadecimal\n"); break;
				case 3: printk(KERN_INFO "Reading format set to binary\n"); break;
				default: printk(KERN_INFO "Reading format is %d\n", read_mode); break;
			}
		}else{printk(KERN_WARNING "Wrong command format - Reading format not changed\n");}
	goto label1;
	}



	for(i=0;i<3;i++){
		if(buff[i] == buff_tmp2[i]){
			ret=1;
		}else{
			ret=0; break;}
	}

	if(ret == 1){
		if(buff[3] == 'A' && buff[4] == '='){
			ret = kstrtoint(buff+5,10,&regA);
			if(!ret) printk(KERN_INFO "Register A set to %d \n", regA);
		}else if(buff[3] == 'B' && buff[4] == '='){
			ret = kstrtoint(buff+5,10,&regB);
			if(!ret) printk(KERN_INFO "Register B set to %d \n", regB);
		      }else if(buff[3] == 'C' && buff[4] == '='){
				ret = kstrtoint(buff+5,10,&regC);
				if(!ret) printk(KERN_INFO "Register C set to %d \n", regC);
			    }else if(buff[3] == 'D' && buff[4] == '='){
					ret = kstrtoint(buff+5,10,&regD);
					if(!ret) printk(KERN_INFO "Register D set to %d \n", regD);
				  }
		if(ret == 0){
			printk(KERN_INFO "Casted successfully \n");
			goto label1;
		}
	}


	i=0;
	while(buff[i] != '\0')
	{
		i++;
	}

	if(i == 9 && ret == 1) //ret = 1 when first 3 characters are reg
	{

	switch(buff[3])
	{
		case 'A': reg1 = regA; break;
		case 'B': reg1 = regB; break;
		case 'C': reg1 = regC; break;
		case 'D': reg1 = regD; break;
		default: reg1 = 0; printk(KERN_INFO"Operand 1 not set\n"); ret=0; break;
	}

	switch(buff[8])
	{
		case 'A': reg2 = regA; break;
		case 'B': reg2 = regB; break;
		case 'C': reg2 = regC; break;
		case 'D': reg2 = regD; break;
		default: reg2 = 0; printk(KERN_INFO "Operand 2 not set\n"); ret=0; break;
	}

	switch(buff[4])
	{
		case '+': result = reg1 + reg2; break;
		case '-': result = reg1 - reg2; break;
		case '*': result = reg1 * reg2; break;
		case '/': result = reg1 / reg2; break;
		default: printk(KERN_INFO "Operation not set\n"); ret=0; break;
	}

	}else{ret = 0;}

	if(!ret) printk(KERN_WARNING "Wrong command format\n");

/*

	//ovo iznad je u kriticnoj sekciji zasticeno semaforom
	up(&sem);povecava semafor za 1 i inicira pozivanje suspendovanih procesa koji cekaju
	na semafor, cime proces izlazi iz kriticne sekcije

	//ovom linijom budimo procese u redu cekanja za citanje, readQ da oslobode mesto u baferu
	wake_up_interruptible(&readQ);
	*/

	label1:

	return length;
}

static int __init alu_init(void)
{
   int ret = 0;

	//inicijalizacija registara na vrednost nula
	regA = 0;
	regB = 0;
	regC = 0;
	regD = 0;
	result = 0;
	carry = 0;

	//sema_init(&sem,1);//semaphore initialization
	/*Inicijalizacija semafora, u globalnu strukturu sem, vrednost 1 oznacava da
	samo jedan proces moze pristupiti semaforu u jednom trenutku, jer se pristupom
	kriticnoj sekciji sem dekrementira sa 1 na 0 i time se sprecava mogucnost drugog
	procesa da izvrsava kod kriticne sekcije jer je ponovnim dekrementiranjem -1, pa
	ce drugi proces biti blokiran dok sem ne bude ponovo 1, odnsono prvi proces ne zavrsi*/


   ret = alloc_chrdev_region(&my_dev_id, 0, 1, "alu");
   if (ret){
      printk(KERN_ERR "failed to register char device\n");
      return ret;
   }
   printk(KERN_INFO "char device region allocated\n");

   my_class = class_create(THIS_MODULE, "alu_class");
   if (my_class == NULL){
      printk(KERN_ERR "failed to create class\n");
      goto fail_0;
   }
   printk(KERN_INFO "class created\n");

   my_device = device_create(my_class, NULL, my_dev_id, NULL, "alu");//ovde kreira node fajl
   if (my_device == NULL){
      printk(KERN_ERR "failed to create device\n");
      goto fail_1;
   }
   printk(KERN_INFO "device created\n");

	my_cdev = cdev_alloc();
	my_cdev->ops = &my_fops;
	my_cdev->owner = THIS_MODULE;
	ret = cdev_add(my_cdev, my_dev_id, 1);
	if (ret)
	{
      printk(KERN_ERR "failed to add cdev\n");
		goto fail_2;
	}
   printk(KERN_INFO "cdev added\n");
   printk(KERN_INFO "Hello world\n");

   return 0;

   fail_2:
      device_destroy(my_class, my_dev_id);
   fail_1:
      class_destroy(my_class);
   fail_0:
      unregister_chrdev_region(my_dev_id, 1);
   return -1;
}

//uklanja node fajl i oslobadja upravljacke brojeve
static void __exit alu_exit(void)
{
   cdev_del(my_cdev);
   device_destroy(my_class, my_dev_id);
   class_destroy(my_class);
   unregister_chrdev_region(my_dev_id,1);
   printk(KERN_INFO "Goodbye, cruel world\n");
}


module_init(alu_init);
module_exit(alu_exit);
