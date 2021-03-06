#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include<QTime>
#include<QFileDialog>
#include<QMessageBox>
#include <../QFFTSpinBox/qfftspinbox.h>
#include <QTime>
#include <bitset>
#include "libwrapper.h"
#include "libwatermark/mathutils/math_util.h"
#include "libwatermark/watermark/RLSBEncode.h"
#include "libwatermark/watermark/RLSBDecode.h"
#include "libwatermark/mathutils/ssw_utils.h"
#include "libwatermark/watermark/SSWEncode.h"
#include "libwatermark/watermark/SSWDecode.h"


// Benchmarks
#include "libwatermark/io/copystyle/InputFilter.h"
#include "libwatermark/io/copystyle/OutputFilter.h"
#include "libwatermark/io/copystyle/InputOLA.h"
#include "libwatermark/io/copystyle/OutputOLA.h"
#include "libwatermark/benchmark/Convolution.h"
#include "libwatermark/benchmark/AddBrumm.h"
#include "libwatermark/benchmark/AddWhiteNoise.h"
#include "libwatermark/benchmark/Amplify.h"
#include "libwatermark/benchmark/Exchange.h"
#include "libwatermark/benchmark/Invert.h"
#include "libwatermark/benchmark/FFTAmplify.h"
#include "libwatermark/benchmark/Smooth.h"
#include "libwatermark/benchmark/ZeroCross.h"
#include "libwatermark/benchmark/Stat1.h"
#include "libwatermark/benchmark/FFTNoise.h"

#include "libwatermark/manager/BenchmarkManager.h"

#include "libwatermark/transform/FFTWManager.h"
#include "libwatermark/io/fftproxy/FFTInputProxy.h"
#include "libwatermark/io/fftproxy/FFTOutputProxy.h"
#include <memory>

#include <QDebug>

/**
 * @brief LibWrapper::LibWrapper
 * Default constructor.
 */
LibWrapper::LibWrapper():
	m_data(new SimpleWatermarkData),
	m_manager(m_data),
	m_settings(this)
{

}

/**
 * @brief LibWrapper::~LibWrapper
 * Destructor.
 */
LibWrapper::~LibWrapper()
{

}

/**
 * @brief LibWrapper::LibWrapper
 * Overloaded constructor to get a pointer to the Graphical User Interface
 * containing every of its elements.
	 * @param gui: pointer to the Graphical User Interface defined with Qt Designer (to link signals etc.)
	 */
	LibWrapper::LibWrapper(Ui::MainWindow* gui):
		LibWrapper()
	{
		m_gui = gui;

	m_gui->availableCapacityLabel->setVisible(false);
	m_gui->availableCapacityLabel2->setVisible(false);

	m_gui->waveformInputWidget->xAxis->setTickLabels(false);
	m_gui->waveformInputWidget->yAxis->setTickLabels(false);
	m_gui->waveformInputWidget->xAxis->setLabel("Input waveform - visible when you load an input audio file");

	m_gui->waveformInputWidget->replot();

	m_gui->waveformOutputWidget->xAxis->setTickLabels(false);
	m_gui->waveformOutputWidget->yAxis->setTickLabels(false);
	m_gui->waveformOutputWidget->xAxis->setLabel("Last output waveform - visible when you encode a watermark");

	m_gui->waveformOutputWidget->replot();

	//m_gui->waveformInputWidget->setVisible(false);
	//m_gui->waveformOutputWidget->setVisible(false);

	//Connecting signals between GUI and watermark library
	connect(m_gui->watermarkSelectionButton,SIGNAL(clicked()),this,SLOT(loadHostWatermarkFile()));
	connect(m_gui->actionLoadHostWatermarkFile,SIGNAL(triggered()),this,SLOT(loadHostWatermarkFile()));
	connect(m_gui->actionLoadFileToDecode, SIGNAL(triggered()), this, SLOT(loadFileToDecode()));
	connect(m_gui->degradationSelect, SIGNAL(clicked()),this,SLOT(loadHostWatermarkFile()));

	connect(m_gui->selectingMethodComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateMethodConfigurationTab(int)));
	connect(m_gui->encodeButton,SIGNAL(clicked()),this,SLOT(encode()));
	connect(m_gui->decodeButton,SIGNAL(clicked()),this,SLOT(decode()));
	connect(m_gui->degradationWrite, SIGNAL(clicked()), this, SLOT(applyDegradation()));

	connect(m_gui->loadConfiguration,SIGNAL(clicked()), &m_settings, SLOT(load()));
	connect(m_gui->saveConfiguration,SIGNAL(clicked()), &m_settings, SLOT(save()));

	connect(m_gui->sswBufferSize, SIGNAL(valueChanged(int)), this, SLOT(setBufferSize(int)));

	connect(m_gui->setDefaultValueLsbPushButton,SIGNAL(clicked()),this,SLOT(setLsbDefaultConfigurationValue()));

	m_gui->actionQuit->setShortcut(tr("CTRL+Q"));
	connect(m_gui->actionQuit,SIGNAL(triggered()),qApp, SLOT(closeAllWindows()));



	connect(m_gui->selectLsbMethodAction, SIGNAL(triggered()),this,SLOT(selectLsbMethodActionSlot()));
	connect(m_gui->selectSswMethodAction, SIGNAL(triggered()),this,SLOT(selectSswMethodActionSlot()));
	connect(m_gui->selectCompExpMethodAction, SIGNAL(triggered()),this,SLOT(selectCompExpMethodActionSlot()));



	connect(m_gui->textToWatermark,SIGNAL(textChanged()),this,SLOT(updateWatermarkCapacityProgressBar()));

	m_gui->usedWatermarkCapacityBar->setStyleSheet(m_ProgressBarSafe);
	m_gui->usedWatermarkCapacityBar->setAlignment(Qt::AlignCenter);

	connect(m_gui->NumberLsbSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateWatermarkCapacityProgressBar()));
	connect(m_gui->lsbMethodUsedComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(updateWatermarkCapacityProgressBar()));

	connect(m_gui->loadWatermarkTextButton,SIGNAL(clicked()),this,SLOT(loadTextWatermarkFile()));

	connect(m_gui->sswGenerate, SIGNAL(clicked()), this, SLOT(generateSSWSequences()));
	//Initializing selection method tab
	m_gui->selectingMethodComboBox->setCurrentIndex(0);
	m_gui->selectingMethodTab->setTabEnabled(0,true);
	m_gui->selectingMethodTab->setTabEnabled(1,false);
	m_gui->selectingMethodTab->setTabEnabled(2,false);

	//Initializing watermark module
	m_gui->watermarkBeginningTime->setEnabled(false);
	m_gui->watermarkEndingTime->setEnabled(false);
	m_gui->usedWatermarkCapacityBar->setEnabled(false);

}

/**
 * @brief LibWrapper::selectLsbMethodActionSlot
 * Function triggered by the action menu to switch
 * between methods
 */
void LibWrapper::selectLsbMethodActionSlot()
{
	m_gui->selectingMethodComboBox->setCurrentIndex(0);
	updateMethodConfigurationTab(0);
}

/**
 * @brief LibWrapper::selectSswMethodActionSlot
 * Function triggered by the action menu to switch
 * between methods
 */
void LibWrapper::selectSswMethodActionSlot()
{
	m_gui->selectingMethodComboBox->setCurrentIndex(1);
	updateMethodConfigurationTab(1);
}

/**
 * @brief LibWrapper::selectCompExpMethodActionSlot
 * Function triggered by the action menu to switch
 * between methods
 */
void LibWrapper::selectCompExpMethodActionSlot()
{
	m_gui->selectingMethodComboBox->setCurrentIndex(2);
	updateMethodConfigurationTab(2);
}


void LibWrapper::plotInput(FileInput<short>* input,unsigned int sr)
{
	int inputLengthInSec = input->frames()/sr;

	QTime inputLength(0,0,0);
	inputLength = inputLength.addSecs(inputLengthInSec);

	//qDebug() << inputLength;

	m_gui->watermarkBeginningTime->setMaximumTime(inputLength);
	m_gui->watermarkEndingTime->setMaximumTime(inputLength);
	m_gui->watermarkEndingTime->setTime(inputLength);

	/* Plotting waveform using QCustomPlot module */

	//
	// TODO: plotting waveform using m_gui->waveformInputWidget
	//

	QVector<double> x,y;

	short min,max;

	x.push_back(0);
	y.push_back(input->v()[0][0]);

	min = y[0];
	max = y[0];

	for(unsigned int i = 1; i < input->frames(); i++)
	{
		x.push_back(i);
		y.push_back(input->v()[0][i]);

		if(y[i] < min) min = y[i];
		if(y[i] > max) max = y[i];
	}

	m_gui->waveformInputWidget->addGraph();
	m_gui->waveformInputWidget->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

	connect(m_gui->waveformInputWidget->xAxis, SIGNAL(rangeChanged(QCPRange)),m_gui->waveformInputWidget->xAxis2, SLOT(setRange(QCPRange)));
	connect(m_gui->waveformInputWidget->yAxis, SIGNAL(rangeChanged(QCPRange)),m_gui->waveformInputWidget->yAxis2, SLOT(setRange(QCPRange)));

	m_gui->waveformInputWidget->graph(0)->setData(x,y);
	m_gui->waveformInputWidget->graph(0)->setName("Input waveform");

	m_gui->waveformInputWidget->xAxis->setRange(0,input->frames());
	m_gui->waveformInputWidget->yAxis->setRange(min,max);

	m_gui->waveformInputWidget->xAxis->setLabel("Input waveform");

	m_gui->waveformInputWidget->replot();

}

/**
 * @brief LibWrapper::loadHostWatermarkFile
 * Function triggered by clicking on the Load Host Watermark
 * button to load the audio file that will host the watermark
 */
void LibWrapper::loadHostWatermarkFile()
{

	m_inputName = QFileDialog::getOpenFileName(m_gui->centralwidget, tr("Open Audio File (.wav)"),
											   "",
											   tr("Audio File (*.wav)"));

	if(!m_inputName.isEmpty())
	{
		//Enabling / Updating watermark module
		m_gui->watermarkBeginningTime->setEnabled(true);
		m_gui->watermarkEndingTime->setEnabled(true);
		m_gui->usedWatermarkCapacityBar->setEnabled(true);

		m_gui->waveformInputWidget->setVisible(true);

		Parameters<short> conf;
		auto input = new FileInput<short>(m_inputName.toStdString(), conf);

		plotInput(input, conf.samplingRate);
		/* Computing audio input time length for initializing editing
		watermark position part */


		/* Editing watermark max length progress bar */

		//m_computedNbFrames = m_nbFramesBase*m_gui->sampleSizeSpinBox->value()/m_sampleSizeBase;
		m_gui->usedWatermarkCapacityBar->setMinimum(0);

		m_nbChannels = input->channels();
		m_nbFrames = input->frames();

		m_gui->availableCapacityLabel2->setVisible(true);

		updateWatermarkCapacityProgressBar();


		m_gui->informationHostWatermark->setText("Opened Host Watermark file:" + m_inputName);
	}
}


void LibWrapper::loadFileToDecode()
{
	m_inputName = QFileDialog::getOpenFileName(m_gui->centralwidget, tr("Open Audio File (.wav)"),
											   "",
											   tr("Audio File (*.wav)"));

	if(!m_inputName.isEmpty())
	{
		m_gui->informationHostWatermark->setText("Opened Host Watermark file:" + m_inputName);
	}
	else
	{
		m_gui->informationHostWatermark->setText("Error, file could not be opened :" + m_inputName);
	}
}

/**
 * @brief LibWrapper::updateMethodConfigurationTab
 * Function triggered by selecting any method tab.
 * @param i: index tab to enable
 */
void LibWrapper::updateMethodConfigurationTab(int i)
{
	if(!m_inputName.isEmpty())
	{
		updateWatermarkCapacityProgressBar();
	}

	switch(i)
	{
		case 0: // lsb method selected
			m_gui->selectingMethodTab->setTabEnabled(0,true);
			m_gui->selectingMethodTab->setTabEnabled(1,false);
			m_gui->selectingMethodTab->setTabEnabled(2,false);
			m_gui->selectingMethodTab->setCurrentIndex(0);

			break;

		case 1: // ssw method selected
			m_gui->selectingMethodTab->setTabEnabled(0,false);
			m_gui->selectingMethodTab->setTabEnabled(1,true);
			m_gui->selectingMethodTab->setTabEnabled(2,false);
			m_gui->selectingMethodTab->setCurrentIndex(1);
			break;

		case 2: // compression-expansion method selected
			m_gui->selectingMethodTab->setTabEnabled(0,false);
			m_gui->selectingMethodTab->setTabEnabled(1,false);
			m_gui->selectingMethodTab->setTabEnabled(2,true);
			m_gui->selectingMethodTab->setCurrentIndex(2);
			break;

		default:
			break;

	}
}

/**
 * @brief LibWrapper::updateWatermarkCapacityProgressBar
 * Function triggered by changing the watermark text to
 * update the capacity progress bar linked
 */

void LibWrapper::updateWatermarkCapacityProgressBar()
{
	if(!m_inputName.isEmpty())
	{
		int i = m_gui->textToWatermark->toPlainText().size();

		switch(m_gui->selectingMethodComboBox->currentIndex())
		{
			case 0: //LSB Method

				switch(m_gui->lsbMethodUsedComboBox->currentIndex())
				{
					case 0: //Simple LSB method
						m_gui->usedWatermarkCapacityBar->setMaximum(m_nbFrames*m_nbChannels*m_gui->NumberLsbSpinBox->value());
						break;

					case 1: //Resistive LSB method
						m_gui->usedWatermarkCapacityBar->setMaximum(m_nbFrames*m_nbChannels);
						break;

					default:
						break;
				}

				m_gui->availableCapacityLabel2->setText(QString::number(i*8 + 64)
														+ '/' + QString::number(m_gui->usedWatermarkCapacityBar->maximum()
																				) + " bits");

				if(i*8 < m_gui->usedWatermarkCapacityBar->maximum())
				{
					m_gui->usedWatermarkCapacityBar->setStyleSheet(m_ProgressBarSafe);
					m_gui->usedWatermarkCapacityBar->setValue(i*8 + 64);
				}
				else
				{
					m_gui->usedWatermarkCapacityBar->setStyleSheet(m_ProgressBarDanger);
					m_gui->usedWatermarkCapacityBar->setValue(m_gui->usedWatermarkCapacityBar->maximum());
				}

				break;

			case 1: //SSW Method
				// Formule: 1 bit par buffer. Donc Num Samples / buffer_size
				// i*8 : nb de bits

				m_gui->usedWatermarkCapacityBar->setMaximum(m_nbFrames * m_nbChannels / sswParams.bufferSize);
				
				m_gui->availableCapacityLabel2->setText(QString::number(i*8 + 64)
														+ '/' + QString::number(m_gui->usedWatermarkCapacityBar->maximum()
																				) + " bits");
				if(i*8 < m_gui->usedWatermarkCapacityBar->maximum())
				{
					m_gui->usedWatermarkCapacityBar->setStyleSheet(m_ProgressBarSafe);
					m_gui->usedWatermarkCapacityBar->setValue(i*8  + 64);
				}
				else
				{
					m_gui->usedWatermarkCapacityBar->setStyleSheet(m_ProgressBarDanger);
					m_gui->usedWatermarkCapacityBar->setValue(m_gui->usedWatermarkCapacityBar->maximum());
				}
				
				break;

			default:

				m_gui->availableCapacityLabel2->setText("Not implemented yet");
				m_gui->usedWatermarkCapacityBar->setValue(m_gui->usedWatermarkCapacityBar->maximum());

				break;

		}
	}
}

/**
 * @brief LibWrapper::dataToBits
 * Function that converts text data to binary sequence data.
 */
void LibWrapper::dataToBits()
{
	auto str = m_gui->textToWatermark->document()->toPlainText().toStdString();

	m_data->setSize(str.size() * 8U); // taille ici

	for (auto i = 0U; i < str.size(); ++i)
	{
		// Ce hack est affreux
		auto a = std::bitset<8>(str[i]);
		auto b = a.to_string();
		std::reverse(std::begin(b), std::end(b));
		auto c = std::bitset<8>(b);

		for(auto i = 0U; i < 8; ++i)
		{
			m_data->setNextBit(c[i]);
		}
	}
}

void LibWrapper::bitsToData()
{
	m_data->readSizeFromBits();
	std::string str = m_data->printBits();
	std::string out;

	for (auto i = 0U; i < str.size(); i += 8)
	{
		auto b = std::bitset<8>(str.substr(i, 8));
		out.push_back(static_cast<unsigned char>(b.to_ulong()));
	}

	m_gui->getDecodedDataTextEdit->setText(QString::fromStdString(out));
}

void LibWrapper::setBufferSize(int size)
{
	sswParams.bufferSize = size;
	updateWatermarkCapacityProgressBar();
}



/**
 * @brief LibWrapper::encode
 * Function triggered by clicking on the Encode button:
 * launch the encoding algorithm thanks to the desired
 * method and parameters.
 */
void LibWrapper::encode()
{

	// host file loaded ? output name correctly defined ?
	if(m_inputName.isEmpty() || !defineSavedFile())
	{
		if(m_inputName.isEmpty())
		{
			m_gui->informationHostWatermark->setText("Error: no Watermark host file defined!");
			QMessageBox::information(m_gui->centralwidget,"Warning - missing file",
									 "Please, load a Watermark host file!");
		}
		return;
	}

	// Displaying if watermark is too heavy for the host file
	if(m_gui->usedWatermarkCapacityBar->value() == m_gui->usedWatermarkCapacityBar->maximum())
	{

		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(m_gui->centralwidget, "Warning - Watermark maybe too big",
									  "Warning: the watermark length is too big to be correctly watermarked into your selected audio file. Do you really want to encode it ?",
									  QMessageBox::Yes|QMessageBox::No);
		if (reply == QMessageBox::No)
		{
			return;
		}
	}

	dataToBits();

	switch(m_gui->selectingMethodTab->currentIndex())
	{
		case 0:
		{
			Parameters<short> conf;

			// Peuvent difficilement être abstraits à cause du <short> (sinon faudrait faire un static_cast et c'est moins propre imho)
			auto input = new FileInput<short>(m_inputName.toStdString(), conf);
			auto output = new FileOutput<short>(conf);


			LSBBase<short>* algorithm = nullptr;
			switch(m_gui->lsbMethodUsedComboBox->currentIndex())
			{

				case 0: //LSB Method
					algorithm = new LSBEncode<short>(conf);
					break;

				case 1: //RLSB Method
					algorithm = new RLSBEncode<short>(conf);
					break;

				default: //Default: LSB
					algorithm = new LSBEncode<short>(conf);
					break;
			}

			algorithm->nbBits = m_gui->NumberLsbSpinBox->value();

			// Mettre à partir d'un certain temps
			auto tb = m_gui->watermarkBeginningTime->time();
			long unsigned bufferBegin = (tb.second() + 60 * tb.minute()) * conf.samplingRate / conf.bufferSize;
			m_manager.timeAdapter() = TimeAdapter_p(new AtTime(bufferBegin));

			m_manager.input() = Input_p(input);
			m_manager.output() = Output_p(output);
			m_manager.algorithm() = Watermark_p(algorithm);

			m_manager.execute();

			output->writeFile(m_outputName.toStdString().c_str());

			drawOutputWaveform(output, conf);
			m_gui->informationHostWatermark->setText("LSB Method: File " + m_outputName +" successfully saved!");

			/* Computing and plotting output waveform */
			// Alban: ça serait bien de mettre ça dans une méthode générique qui prend un IOInterface* en paramètre
			// Ou alors vu qu'il y a des shorts, le mettre en template sur un OutputManagerBase<T>.

			break;
		}
		case 1:
		{
			auto input = new FileInput<double>(m_inputName.toStdString(), sswParams);
			auto output = new FileOutput<double>(sswParams);

			FFT_p<double> fft_m(new FFTWManager<double>(sswParams));
			fft_m->setChannels((unsigned int) input->channels());
			auto fft_i = new FFTInputProxy<double>(input, fft_m, sswParams);
			auto fft_o = new FFTOutputProxy<double>(output, fft_m, sswParams);

			auto VariantToInt = [] (QVariant x) { return x.toInt(); };
			// Récupération des séquences
			std::vector<int> PNSequence(m_gui->sswSeqSize->value());
			std::vector<unsigned int> FreqRange(m_gui->sswSeqSize->value());
			auto v = m_settings.readSSWLine(0);
			auto w = m_settings.readSSWLine(1);
			std::transform(v.begin(), v.end(), PNSequence.begin(), VariantToInt);
			std::transform(w.begin(), w.end(), FreqRange.begin(), VariantToInt);

			double ampl = m_gui->sswAmpl->value();

			std::cerr << ampl << " ";

			auto algorithm = new SSWEncode<double>(sswParams, PNSequence, FreqRange, ampl);

			// Mettre à partir d'un certain temps
			auto tb = m_gui->watermarkBeginningTime->time();
			long unsigned bufferBegin = (tb.second() + 60 * tb.minute()) * sswParams.samplingRate / sswParams.bufferSize;
			m_manager.timeAdapter() = TimeAdapter_p(new AtTime(bufferBegin));
			m_manager.input() = Input_p(fft_i);
			m_manager.output() = Output_p(fft_o);
			m_manager.algorithm() = Watermark_p(algorithm);

			m_manager.execute();

			output->writeFile(m_outputName.toStdString().c_str());

			drawOutputWaveform(output, sswParams);
			m_gui->informationHostWatermark->setText("SSW Method: File " + m_outputName +" successfully saved!");
			break;
		}
			//case 2:
			// Rien pour l'instant
			//break;
		default:
			m_gui->informationHostWatermark->setText("Warning: method not implemented yet");
			QMessageBox::information(m_gui->centralwidget,"Warning - method",
									 "This method is not yet implemented!");
			break;
	}
}


/**
 * @brief LibWrapper::decode
 * Function triggered by clicking on the Decode button:
 * launch the decoding algorithm thanks to the desired
 * method and parameters.
 */
void LibWrapper::decode()
{
	m_data->resetData();
	if(m_inputName.isEmpty())
	{
		m_gui->informationHostWatermark->setText("Error: no Watermark host file defined!");
		QMessageBox::information(m_gui->centralwidget,"Warning - missing file",
								 "Please, load a Watermark host file!");

		return;
	}

	switch(m_gui->selectingMethodTab->currentIndex())
	{
		case 0:
		{
			Parameters<short> conf;

			auto input = new FileInput<short>(m_inputName.toStdString(), conf);
			auto output = new DummyOutput<short>(conf);

			LSBBase<short>* algorithm = nullptr;
			switch(m_gui->lsbMethodUsedComboBox->currentIndex())
			{
				case 0: //LSB Method
					algorithm = new LSBDecode<short>(conf);
					break;

				case 1: //RLSB Method
					algorithm = new RLSBDecode<short>(conf);
					break;

				default: //Default: LSB
					algorithm = new LSBDecode<short>(conf);
					break;
			}

			algorithm->nbBits = m_gui->NumberLsbSpinBox->value();

			// Mettre à partir d'un certain temps
			auto tb = m_gui->watermarkBeginningTime->time();
			long unsigned bufferBegin = (tb.second() + 60 * tb.minute()) * conf.samplingRate / conf.bufferSize;
			m_manager.timeAdapter() = TimeAdapter_p(new AtTime(bufferBegin));
			m_manager.input() = Input_p(input);
			m_manager.output() = Output_p(output);
			m_manager.algorithm() = Watermark_p(algorithm);

			m_manager.execute();

			m_gui->informationHostWatermark->setText("LSB Method: File " + m_inputName +" successfully read!");

			break;
		}
		case 1:
		{
			auto input = new FileInput<double>(m_inputName.toStdString(), sswParams);
			auto output = new DummyOutput<double>(sswParams);

			FFT_p<double> fft_m(new FFTWManager<double>(sswParams));
			fft_m->setChannels((unsigned int) input->channels());
			auto fft_i = new FFTInputProxy<double>(input, fft_m, sswParams);
			auto fft_o = new FFTOutputProxy<double>(output, fft_m, sswParams);


			auto VariantToInt = [] (QVariant x) { return x.toInt(); };
			// Récupération des séquences
			std::vector<int> PNSequence(m_gui->sswSeqSize->value());
			std::vector<unsigned int> FreqRange(m_gui->sswSeqSize->value());
			auto v = m_settings.readSSWLine(0);
			auto w = m_settings.readSSWLine(1);
			std::transform(v.begin(), v.end(), PNSequence.begin(), VariantToInt);
			std::transform(w.begin(), w.end(), FreqRange.begin(), VariantToInt);

			double ampl = m_gui->sswAmpl->value();
			double thres = m_gui->sswThres->value();

			auto algorithm = new SSWDecode<double>(sswParams, PNSequence, FreqRange, ampl, thres);

			// Mettre à partir d'un certain temps
			auto tb = m_gui->watermarkBeginningTime->time();
			long unsigned bufferBegin = (tb.second() + 60 * tb.minute()) * sswParams.samplingRate / sswParams.bufferSize;
			m_manager.timeAdapter() = TimeAdapter_p(new AtTime(bufferBegin));
			m_manager.input() = Input_p(fft_i);
			m_manager.output() = Output_p(fft_o);
			m_manager.algorithm() = Watermark_p(algorithm);

			m_manager.execute();

			m_gui->informationHostWatermark->setText("SSW Method: File " + m_inputName +" successfully read!");
			break;
		}
			//case 2:
			// Rien pour l'instant
			//break;
		default:
			m_gui->informationHostWatermark->setText("Warning: method not implemented yet");
			QMessageBox::information(m_gui->centralwidget,"Warning - method",
									 "This method is not yet implemented!");
			break;
	}

	bitsToData();
}

void LibWrapper::generateSSWSequences()
{
	int size = m_gui->sswSeqSize->value();
	auto PNSequence = SSWUtil::generatePNSequence(size);
	auto FrequencyRange = SSWUtil::generateFrequencyRange(size, sswParams);

	for(int i = 0; i < size; i++)
	{
		m_gui->sswSequences->removeColumn(i);
		m_gui->sswSequences->insertColumn(i);
		m_gui->sswSequences->setItem(0, i, new QTableWidgetItem(QString::number(PNSequence[i])));
		m_gui->sswSequences->setItem(1, i, new QTableWidgetItem(QString::number(FrequencyRange[i])));
	}
}

void LibWrapper::applyDegradation()
{
	if(m_inputName.isEmpty() || !defineSavedFile())
	{
		if(m_inputName.isEmpty())
		{
			QMessageBox::information(m_gui->centralwidget,"Warning - missing file",
									 "Please, load an audio file!");
		}
		return;
	}

	Parameters<double> conf;

	BenchmarkBase<double>* algorithm = nullptr;
	InputCopy<double>* iCp = nullptr;
	OutputCopy<double>* oCp = nullptr;

	Input_p input;
	Output_p output;
	switch(m_gui->degradationMethod->currentIndex())
	{
		case 0:
			algorithm = new AddBrumm<double>(conf);
			break;
		case 1:
			algorithm = new AddWhiteNoise<double>(conf);
			break;
		case 2:
			algorithm = new Amplify<double>(conf);
			break;
		case 3:
			algorithm = new Convolution<double>(conf);
			break;
		case 4:
			algorithm = new Exchange<double>(conf);
			break;
		case 5:
			algorithm = new FFTAmplify<double>(conf);
			break;
		case 6:
			algorithm = new FFTNoise<double>(conf);
			break;
		case 7:
			algorithm = new Invert<double>(conf);
			break;
		case 8:
			algorithm = new Smooth<double>(conf);
			break;
		case 9:
			algorithm = new Stat1<double>(conf);
			break;
		case 10:
			algorithm = new ZeroCross<double>(conf);
			break;
	}

	// On set les paramètres
	if (auto ap = dynamic_cast<AmplitudeProperty*>(algorithm))
		ap->_amplitude = m_gui->degradationAmplitude->value();

	if (auto fp = dynamic_cast<FrequencyProperty*>(algorithm))
		fp->_frequency = m_gui->degradationAmplitude->value();

	if (auto tp = dynamic_cast<ThresholdProperty*>(algorithm))
		tp->_threshold = m_gui->degradationAmplitude->value();

	if(dynamic_cast<FilterProperty*>(algorithm))
	{
		iCp = new InputFilter<double>(11, conf);
		oCp = new OutputFilter<double>(11, conf);
	}
	else
	{
		iCp = new InputSimple<double>(conf);
		oCp = new OutputSimple<double>(conf);
	}

	auto fileInput = new FileInput<double>(m_inputName.toStdString(), iCp, conf);
	auto fileOutput = new FileOutput<double>(oCp, conf);


	if(dynamic_cast<FFTProperty*>(algorithm))
	{
		FFT_p<double> fft_m(new FFTWManager<double>(conf));
		fft_m->setChannels((unsigned int) fileInput->channels());
		auto fft_i = new FFTInputProxy<double>(fileInput, fft_m, conf);
		auto fft_o = new FFTOutputProxy<double>(fileOutput, fft_m, conf);

		input.reset(fft_i);
		output.reset(fft_o);
	}
	else
	{
		input.reset(fileInput);
		output.reset(fileOutput);
	}

	BenchmarkManager b_manager(input, output, Benchmark_p(algorithm));

	b_manager.execute();

	fileOutput->writeFile(m_outputName.toStdString().c_str());
}

/**
 * @brief LibWrapper::defineSavedFile
 * @return boolean: true if output name is correct
 * false otherwise
 */
bool LibWrapper::defineSavedFile()
{
	m_outputName = QFileDialog::getSaveFileName(this, tr("Save Watermarked Output File (.wav)"),
												"./",
												tr("Audio File (*.wav)"));

	if(!m_outputName.isEmpty())
	{
		if(!m_outputName.endsWith(".wav"))
			m_outputName.append(".wav");
		return true;
	}
	else
		return false;
}

/**
 * @brief LibWrapper::setLsbDefaultConfigurationValue
 * Function triggered by the set default config value
 * button (for LSB tab): resets configuration at default
 * values
 */
void LibWrapper::setLsbDefaultConfigurationValue()
{
	m_gui->NumberLsbSpinBox->setValue(1);
}

/**
 * @brief LibWrapper::setSswDefaultConfigurationValue
 * Function triggered by the set default config value
 * button (for SSW tab): resets configuration at default
 * values
 */
void LibWrapper::setSswDefaultConfigurationValue()
{
	m_settings.subLoad("defaultssw.ini");
}

/**
 * @brief LibWrapper::setCompExpDefaultConfigurationValue
 * Function triggered by the set default config value
 * button (for Compression-Expansion tab): resets
 * configuration at default values
 */
void LibWrapper::setCompExpDefaultConfigurationValue()
{

}

void LibWrapper::loadTextWatermarkFile()
{
	QString tempFile = QFileDialog::getOpenFileName(m_gui->centralwidget, tr("Open text watermark file (.txt)"),
													"",
													tr("Text Watermark File (*.txt)"));

	if(!tempFile.isEmpty())
	{
		QFile file(tempFile);

		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			return;

		QTextStream in(&file);

		m_gui->textToWatermark->clear();
		m_gui->textToWatermark->appendPlainText(in.readAll());

		m_gui->informationHostWatermark->setText("Opened Text Watermark File:" + tempFile);
	}
}
