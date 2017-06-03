#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>
#include <lusb0_usb.h>
#include "opendevice.h"

#define READBUFF_SIZE 1024
#define ADC_SIZE 512

void initialize(void);
void keyboardFunc(unsigned char key, int x, int y);
void displayFunc(void);
void animate(int value);
void drawSeries(unsigned char* values, int size, int last);
void drawBinary(unsigned char* values, int size);

int timer_ms = 20;
char mode = 2;
float zoomY = 1.0f;
float shiftY = 0.0f;

usb_dev_handle  *handle = NULL;
unsigned char readBuffer[READBUFF_SIZE];
int len = 0;
unsigned char adc[ADC_SIZE];
int curr_adc = 0;

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitWindowSize(600, 200);
	glutInitWindowPosition(200, 200);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutCreateWindow("oscilloscope");
	initialize();
	glutKeyboardFunc(&keyboardFunc);
	glutDisplayFunc(&displayFunc);
	glutTimerFunc(timer_ms, &animate, 0);
	glutMainLoop();
	return EXIT_SUCCESS;
}

void initialize() {
	glClearColor(1.0, 1.0, 1.0, 0.0);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-0.01, 1.02, -0.01, 1.02, -1.0, 1.0);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void keyboardFunc(unsigned char key, int x, int y) {
	switch (key) {
		case '\x1B':
			exit(EXIT_SUCCESS);
			break;
		case 'd':
			displayFunc();
			break;
		case 'a':
			mode = 2;
			break;
		case ';': //1.1V reference
			len = usb_control_msg(handle, 192, 1, 1, 0, NULL, 0, 5000);
			break;
		case '\'': //Vcc reference
			len = usb_control_msg(handle, 192, 1, 0, 0, NULL, 0, 5000);
			break;
		case '[':
			timer_ms--;
			if (timer_ms <= 0)
				timer_ms = 0;
			break;
		case ']':
			timer_ms++;
			break;
		case 'p':
			if (mode == 0)
				mode = 2;
			else
				mode = 0;
			break;
		case 'o':
			glutSetWindowTitle("sampling...");
			len = usb_control_msg(handle, 192, 3, 0, 0, NULL, 0, 5000);
			Sleep(500);
			glutSetWindowTitle("getting result...");
			len = usb_control_msg(handle, 192, 6, 0, 0, (char*)readBuffer, READBUFF_SIZE, 5000);
			mode = 3;
			break;
		case 'f':
			glutSetWindowTitle("sampling...");
			len = usb_control_msg(handle, 192, 19, 0, 0, NULL, 0, 5000);
			Sleep(1000);
			glutSetWindowTitle("getting result...");
			len = usb_control_msg(handle, 192, 6, 0, 0, (char*)readBuffer, READBUFF_SIZE, 5000);
			mode = 3;
			break;
		case 'F':
			glutSetWindowTitle("sampling...");
			len = usb_control_msg(handle, 192, 20, 0, 0, NULL, 0, 5000);
			Sleep(7000);
			glutSetWindowTitle("getting result...");
			len = usb_control_msg(handle, 192, 6, 0, 0, (char*)readBuffer, READBUFF_SIZE, 5000);
			mode = 3;
			break;
		case 'h':
			glutSetWindowTitle("sampling...");
			len = usb_control_msg(handle, 192, 4, 0, 0, NULL, 0, 5000);
			Sleep(500);
			glutSetWindowTitle("getting result...");
			len = usb_control_msg(handle, 192, 6, 0, 0, (char*)readBuffer, READBUFF_SIZE, 5000);
			mode = 4;
			break;
		case 'r':
			//glutSetWindowTitle("wait for it...");
			//Sleep(5000);
			glutSetWindowTitle("sampling...");
			len = usb_control_msg(handle, 192, 18, 0, 0, NULL, 0, 5000);
			Sleep(500);
			glutSetWindowTitle("getting result...");
			len = usb_control_msg(handle, 192, 6, 0, 0, (char*)readBuffer, READBUFF_SIZE, 5000);
			mode = 4;
			break;
		case '=':
			zoomY = 2.0f;
			shiftY = -0.5f;
			break;
		case '-':
			zoomY = 1.0f;
			shiftY = 0.0f;
			break;
		case '2':
			len = usb_control_msg(handle, 192, 17, 0x82, 0, NULL, 0, 5000);
			break;
		case '5':
			len = usb_control_msg(handle, 192, 17, 0x85, 0, NULL, 0, 5000);
			break;
	}
}

void animate(int value) {
	glutTimerFunc(timer_ms, &animate, 0);
	glutPostRedisplay();
	if (handle == NULL) {
		glutSetWindowTitle("Usb error!!! Check device or driver.");
		if (usbOpenDevice(&handle, 0, "obdev*", 0, NULL, NULL, NULL, NULL) != 0){
			return;
		}
	}

	switch (mode) {
		case 0:
			glutSetWindowTitle("paused");
			break;
		case 2:
			{
				char title[128];
				float v_average = adc[curr_adc];
				v_average += ((curr_adc == 0)?adc[ADC_SIZE-1]:adc[curr_adc-1]);
				v_average += ((curr_adc == 0)?adc[ADC_SIZE-2]:adc[curr_adc-2]);
				v_average += ((curr_adc == 0)?adc[ADC_SIZE-3]:adc[curr_adc-3]);
				v_average += ((curr_adc == 0)?adc[ADC_SIZE-4]:adc[curr_adc-4]);
				v_average /= 5;
				//sprintf(title, "scoping %i ms, (%.3f V / %.3f V)", timer_ms, adc[curr_adc]*3.9/255.0, adc[curr_adc]*1.1/255.0);
				sprintf(title, "scoping %i ms, (%.3f V / %.3f V)", timer_ms, v_average*3.37/255.0, v_average*1.08/255.0);
				glutSetWindowTitle(title);
			}

			len = usb_control_msg(handle, 192, 2, 0, 0, (char*)readBuffer, 128, 5000);
			if(len < 0){
				fprintf(stderr, "USB error: %s\n", usb_strerror());
				handle = NULL;
			}
			curr_adc++;
			if (curr_adc >= ADC_SIZE)
				curr_adc = 0;
			adc[curr_adc] = (unsigned char)(readBuffer[0]);
			break;
		case 3:
		case 4:
			glutSetWindowTitle("sampled");
			break;
	}
}

void displayFunc() {
	glClear(GL_COLOR_BUFFER_BIT);
	glLineWidth(1.5f);
	glColor3f(0.0f, 0.0f, 0.0f);
	glMatrixMode(GL_PROJECTION);

	switch (mode) {
		case (2):
			drawSeries(adc, ADC_SIZE, curr_adc);
			break;
		case (3):
			drawSeries(readBuffer, READBUFF_SIZE, 0);	
			break;
		case (4):
			drawBinary(readBuffer, READBUFF_SIZE/8);
			break;
	}
	
	glutSwapBuffers();
}


void drawSeries(unsigned char* values, int size, int last) {
	glPushMatrix();
	glTranslatef(0.0f, shiftY, 0.0f);
	glScalef(1.0f, zoomY, 1.0f);
	glBegin(GL_LINE_STRIP);
		int i = last+1;
		if (i >= size)
			i = 0;
		while (i != last) {
			int tempi = 0;
			if (i <= last)
				tempi = size - last + i;
			else
				tempi = i - last;
			double x = tempi/(double)(size);
			double y = values[i]/255.0;
			glVertex2f(x, y);
			i++;
			if (i >= size)
				i = 0;
		}
	glEnd();
	glPopMatrix();
}

void drawBinary(unsigned char* values, int size) {
	glPushMatrix();
	glTranslatef(0.0f, shiftY, 0.0f);
	glScalef(1.0f, zoomY, 1.0f);
	glBegin(GL_LINE_STRIP);
		int i = 0;
		while (i < size) {
			char bit = 0;
			while (bit < 8) {
				double x = (8*i + bit)/(double)(size*8);
				double y = (1 & (values[i]>>bit));
				glVertex2f(x, y);
				x = (8*i + bit+1)/(double)(size*8);
				glVertex2f(x, y);
				bit++;
			}
			i++;
		}
	glEnd();
	glPopMatrix();
}
