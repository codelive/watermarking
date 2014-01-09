//#pragma once
#ifndef SSW_UTILS_H
#define SSW_UTILS_H

#include <random>
#include <vector>
#include <algorithm>
namespace SSWUtil
{
	/* Génère une séquence pseudo-aléatoire à valeurs comprises dans {-1,1}
	   de taille size */
        std::vector<int> generatePNSequence(int size)
        {
			// Générer séquence de nombres entre 0 et 1
			// Méthode C++
			std::default_random_engine rng(std::random_device{}());
			std::uniform_int_distribution<int> dist(0, 1);

			std::vector<int> pnsequence(size);
			std::generate_n(pnsequence.begin(), size, [&] ()
			{
				return dist(rng) * 2 - 1;
			});

			return pnsequence;

			/* Méthode C
			srand(time(0));
            for(int i = 0; i < size; i++)
            {
                int j = rand()%2;
                if(j == 0)
                    pnsequence.push_back(-1);
                else
                    pnsequence.push_back(1);
            }
			*/
        }

        /* Génère une plage de fréquence aléatoire de taille size et comprise dans
			l'intervalle [1,sampleRate] (pourquoi pas sampleRate / 2 ?) */
		std::vector<unsigned int> generateFrequencyRange(unsigned int size, unsigned int sampleRate)
        {
			/*
			std::default_random_engine rng(std::random_device{}());
			std::uniform_int_distribution<int> dist(1, sampleRate);
			*/
			std::vector<unsigned int> range;
			for (unsigned int i = 0; i < size; i++) {
				range.push_back(i+30);
			}
			/*
			std::generate_n(range.begin(), size, [&] ()
			{
				return dist(rng);
			});
			*/

			return range;

			/*
			srand(time(0));
			std::vector<int> frequencyRange;

            int j = (rand()%(sampleRate-size))+1;
            frequencyRange.push_back(j);

            for(int i = 1; i < size; i++)
			{
				frequencyRange.push_back(i+j);
            }

            return frequencyRange;
			*/
        }
}

#endif
