
/*
Zadatak utorak 18h treba napraviti FIFO buffer sa 16 mesta velicine izmedju 0 i 255
upis novih brojeva se vrsi u binarnom formatu echo 0b00001111;0b11110000; /dev/fifo
citanje brojeva iz bafera se vrsi u fifo maniru sa komandom cat fifo, echo num=n>/dev/fifo
oznacava da se iscita n brojeva u fifo maniru
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
#define BUFF_SIZE_IN 200
//BUFF_SIZE_IN odredjuje velicinu niza za pocetni ulazni string koji obradjujemo

MODULE_LICENSE("Dual BSD/GPL");

dev_t my_dev_id;
static struct class *my_class;
static struct device *my_device;
static struct cdev *my_cdev;

int fifo[16];
int pos = 0;
int endRead = 0;
int n = 1;//read configuration parameter, reading n elements
int read_counter = 0;//counting number of read iterations set by n


int fifo_open(struct inode *pinode, struct file *pfile);
int fifo_close(struct inode *pinode, struct file *pfile);
ssize_t fifo_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset);
ssize_t fifo_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset);

struct file_operations my_fops =
{
	.owner = THIS_MODULE,
	.open = fifo_open,
	.read = fifo_read,
	.write = fifo_write,
	.release = fifo_close,
};


int fifo_open(struct inode *pinode, struct file *pfile) 
{
		printk(KERN_INFO "Succesfully opened fifo\n");
		return 0;
}

int fifo_close(struct inode *pinode, struct file *pfile) 
{
		printk(KERN_INFO "Succesfully closed fifo\n");
		return 0;
}

ssize_t fifo_read(struct file *pfile, char __user *buffer, size_t length, loff_t *offset) 
{
	int ret;
	char buff[BUFF_SIZE];
	long int len = 0;
	int i;

	if (endRead){
		endRead = 0;
		return 0;
	}

	//proces zauzima semafor
	if(down_interruptible(&sem))
		return -ERESTARTSYS;
	while(pos == 0)
	{
		/*ulazi while petlju, oslobadja semafor kako bi neki drugi proces mogao da upise i
		blokira se u redu cekanja citanja*/
		up(&sem);
		/*Ukoliko nista nije upisano proces se blokira u ovoj liniji i smesta u red cekanja
		za citanje readQ */
		if(wait_event_interruptible(readQ,(pos>0)))
			return -ERESTARTSYS;
		/*Nakon budjenja procesa koji cekaju u redu za cigtanje readQ ova funkcija
		nastavlja da se izvrsava odavde. Proces citanja se budi na kraju funkcije za upis,
		gde se poziva funkcija za budjenje procesa citanja wake_up_interruptible(&readQ)*/

		//ponovo zauzima semafor nakon sto je probudjen iz reda cekanja na citanje
		if(down_interruptible(&sem))
			return -ERESTARTSYS;
	}//while(pos==0)

	if(pos > 0)
	{
		//pos--; dekrementiranje ostalo od lifo buffera
		len = scnprintf(buff, BUFF_SIZE, "%d ", fifo[0]);
		ret = copy_to_user(buffer, buff, len);

		if(ret)
			return -EFAULT;
		printk(KERN_INFO "Succesfully read\n");//uspesno procitan podatak fifo[0]

		read_counter++;//counting number of reads
		if(read_counter == n){
			endRead = 1;
			read_counter = 0;
		}
		//until number of reads reach n endRead remains 0 and cat command keeps reading
		//cat command will keep calling again until len doesnt come to 0 which is EOF

		//for petljom pomeri sve clanove niza fifo za jedno mesto ulevo nakon 1 procitanog
		for(i=0;i<pos;i++){
			fifo[i] = fifo[i+1];
		}
		pos--;//nakon pomeranja svih clanova niza za jedno mesto ulevo smanji poziciju


	}
	else
	{
			printk(KERN_WARNING "Fifo is empty\n"); 
	}

	up(&sem);//oslobadanje semafora izlazak iz kriticne sekcije
	wake_up_interruptible(&writeQ);//ova linija budi procese koji cekaju u redu za upis
	return len;
}

ssize_t fifo_write(struct file *pfile, const char __user *buffer, size_t length, loff_t *offset) 
{
	char buff[BUFF_SIZE_IN];
	char buff_tmp[20];
	char buff_tmp2[4] = {'n','u','m','='};
	int value;
	int ret;
	int i, j, k;

	ret = copy_from_user(buff, buffer, length);//successful execution returns 0
	/*buff je kernel niz gde se smesta buffer je niz u kurisnickom prostoru,
	length je duzina niza koji se kopira*/


	if(ret)
		return -EFAULT;

	buff[length-1] = '\0'; //niz krece od nula a length je ukupan broj clanova niza buffer

	for(i=0;i<4;i++){
		if(buff[i] == buff_tmp2[i]){
			ret=1;
		}else{
			ret=0; break;}
	}

	if(ret == 1){
		ret = kstrtoint(buff+4,10,&n);//successful cast returns 0
		if(ret == 0){
			printk(KERN_INFO "Set to read n = %d elements\n", n);
			goto label1;
		}else{
			ret = 1;
			printk(KERN_INFO "Wrong command format\n");
		}
	}


	if(down_interruptible(&sem))
		return -ERESTARTSYS;
	while(pos == 16)
	{
		up(&sem);//oslobadjanje semafora JAKO VAZNO! da ne bi blokirao sve ostale procese

		/*ukoliko je buffer pun, proces smestamo u red cekanja za upis, writeQ i blokira se
		u ovoj liniji*/
		if(wait_event_interruptible(writeQ,(pos<16)))
			return -ERESTARTSYS;
		/*
		Kada se procita podatak i oslobodi mesto iz buffera proces se budi na kraju read
		funkcije sa wake_up_interruptible(writeQ) i nastavlja odavde upis
		*/
		if(down_interruptible(&sem))
			return -ERESTARTSYS;
	}

	if(pos<16)
	{
		//ret = sscanf(buff,"%d",&value);
		while(buff[0] != '\0'){

			i = 0;
			while(buff[i] != ';'){
				buff_tmp[i] = buff[i];
				i++;
				if(buff_tmp[i-1] == '\0' || i == 19){
					buff_tmp[0]='e';//error bit disables if(0b || 0B)
					break;
					}//safety feature buff_tmp size is 20, buff 200
			}
			buff_tmp[i] = '\0';


			if(buff_tmp[0] == '0' && (buff_tmp[1] == 'b' || buff_tmp[1] == 'B')){
				ret = kstrtoint(buff_tmp+2,2,&value);
				printk(KERN_INFO "Izdvojeni string %s", buff_tmp);
			}else{
				ret = 1;
			}
			//shift whole array 1 space left, for i times (i = position of ; character)
			j=0;
			for(j=0;j<=i;j++){
				k=0;
				while(buff[k] != '\0'){
					buff[k] = buff[k+1];
					k++;
				}
			}

			//Checking if number casted right ret =0 and in range 0 to 255, 8bit
			if(ret == 0 && (value > 255 || value < 0))
			{
				ret = 1;//if not in range, wont be written into buffer
				printk(KERN_WARNING "8 bit maximum range exceeded, data discarded");
			}

			if(ret == 0)//one parameter parsed from input string
			{
				printk(KERN_INFO "Succesfully wrote value %d", value);
				fifo[pos] = value;
				pos=pos+1;
			}
			else
			{
				printk(KERN_WARNING "Wrong command format\n");//ret = 1
			}

		}//while
	}//if(pos<16)
	else
	{
		printk(KERN_WARNING "Fifo is full\n"); 
	}//else(pos<16)


	//ovo iznad je u kriticnoj sekciji zasticeno semaforom
	up(&sem);/*povecava semafor za 1 i inicira pozivanje suspendovanih procesa koji cekaju
	na semafor, cime proces izlazi iz kriticne sekcije*/

	//ovom linijom budimo procese u redu cekanja za citanje, readQ da oslobode mesto u baferu
	wake_up_interruptible(&readQ);

	label1:

	return length;
}

static int __init fifo_init(void)
{
   int ret = 0;
	int i=0;

	sema_init(&sem,1);//semaphore initialization
	/*Inicijalizacija semafora, u globalnu strukturu sem, vrednost 1 oznacava da
	samo jedan proces moze pristupiti semaforu u jednom trenutku, jer se pristupom
	kriticnoj sekciji sem dekrementira sa 1 na 0 i time se sprecava mogucnost drugog
	procesa da izvrsava kod kriticne sekcije jer je ponovnim dekrementiranjem -1, pa
	ce drugi proces biti blokiran dok sem ne bude ponovo 1, odnsono prvi proces ne zavrsi*/

	//Initialize array
	for (i=0; i<16; i++)
		fifo[i] = 0;


   ret = alloc_chrdev_region(&my_dev_id, 0, 1, "fifo");
   if (ret){
      printk(KERN_ERR "failed to register char device\n");
      return ret;
   }
   printk(KERN_INFO "char device region allocated\n");

   my_class = class_create(THIS_MODULE, "fifo_class");
   if (my_class == NULL){
      printk(KERN_ERR "failed to create class\n");
      goto fail_0;
   }
   printk(KERN_INFO "class created\n");

   my_device = device_create(my_class, NULL, my_dev_id, NULL, "fifo");//ovde kreira node fajl
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
static void __exit fifo_exit(void)
{
   cdev_del(my_cdev);
   device_destroy(my_class, my_dev_id);
   class_destroy(my_class);
   unregister_chrdev_region(my_dev_id,1);
   printk(KERN_INFO "Goodbye, cruel world\n");
}


module_init(fifo_init);
module_exit(fifo_exit);
