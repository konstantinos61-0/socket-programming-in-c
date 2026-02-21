#include "server.h"

void fill_transitions(int trans[STATES - 2][INPUTS])
{ 
    // transition table definition
    for (int i = 0; i < STATES - 2; i++)
    {
        for (int j = 0; j < INPUTS; j++)
        {
            if (i == M)
            {
                if (j == 'G')
                    trans[i][j] = M_G;
                else if (j == 'P')
                    trans[i][j] = M_P;
                else if (j == 'H')
                    trans[i][j] = M_H;
                else
                    trans[i][j] = FAILURE;
            }
            else if (i == M_G)
            {
                if (j == 'E')
                    trans[i][j] = M_E;
                else
                    trans[i][j] = FAILURE;
            }
            else if (i == M_E)
            {
                if (j == 'T')
                    trans[i][j] = M_T;
                else
                    trans[i][j] = FAILURE;
            }
            else if (i == M_T)
            {
                if (j == ' ')
                    trans[i][j] = URI;
                else   
                    trans[i][j] = FAILURE;
            }
            else if (i == M_P)
            {
                if (j == 'O')
                    trans[i][j] = M_O;
                else
                    trans[i][j] = FAILURE;  
            }
            else if (i == M_O)
            {
                if (j == 'S')
                    trans[i][j] = M_S;
                else
                    trans[i][j] = FAILURE;  
            }
            else if (i == M_S)
            {
                if (j == 'T')
                    trans[i][j] = M_T2;
                else
                    trans[i][j] = FAILURE;  
            }
            else if (i == M_T2)
            {
                if (j == ' ')
                    trans[i][j] = URI;
                else
                    trans[i][j] = FAILURE;  
            }
            else if (i == M_H)
            {
                if (j == 'E')
                    trans[i][j] = M_E2;
                else
                    trans[i][j] = FAILURE;
            }
            else if (i == M_E2)
            {
                if (j == 'A')
                    trans[i][j] = M_A;
                else
                    trans[i][j] = FAILURE;
            }
            else if (i == M_A)
            {
                if (j == 'D')
                    trans[i][j] = M_D;
                else
                    trans[i][j] = FAILURE;
            }
            else if (i == M_D)
            {
                if (j == ' ')
                    trans[i][j] = URI;
                else
                    trans[i][j] = FAILURE;
            }
            else if (i == URI)
            {
                if (j == '/')
                    trans[i][j] = URI_CHAR;
                else
                    trans[i][j] = FAILURE;
            }
            else if (i == URI_CHAR)
            {
                if (j == ' ')
                    trans[i][j] = V;
                else 
                    trans[i][j] = URI_CHAR;   
            }
            else if (i == V)
            {
                if (j == 'H')
                    trans[i][j] = V_H;
                else
                    trans[i][j] = FAILURE;
            }
            else if (i == V_H)
            {
                if (j == 'T')
                    trans[i][j] = V_T1;
                else 
                    trans[i][j] = FAILURE;
            }
            else if (i == V_T1)
            {
                if (j == 'T')
                    trans[i][j] = V_T2;
                else 
                    trans[i][j] = FAILURE;
            }
            else if (i == V_T2)
            {
                if (j == 'P')
                    trans[i][j] = V_P;
                else 
                    trans[i][j] = FAILURE;
            }
            else if (i == V_P)
            {
                if (j == '/')
                    trans[i][j] = V_SLASH;
                else 
                    trans[i][j] = FAILURE;
            }
            else if (i == V_SLASH)
            {
                if (j == '1')
                    trans[i][j] = V_MAJOR;
                else 
                    trans[i][j] = FAILURE;
            }
            else if (i == V_MAJOR)
            {
                if (j == '.')
                    trans[i][j] = V_DOT;
                else 
                    trans[i][j] = FAILURE;
            }
            else if (i == V_DOT)
            {
                if (j == '1' || j == '0')
                    trans[i][j] = V_MINOR;
                else 
                    trans[i][j] = FAILURE;
            }
            else if (i == V_MINOR)
            {
                if (j == '\r')
                    trans[i][j] = CR;
                else 
                    trans[i][j] = FAILURE;
            }
            else if (i == CR)
            {
                if (j == '\n')
                    trans[i][j] = SUCCESS;
                else 
                    trans[i][j] = FAILURE;
            }
        }
    }
}