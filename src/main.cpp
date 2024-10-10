/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mvillarr <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/10/10 15:56:39 by mvillarr          #+#    #+#             */
/*   Updated: 2024/10/10 16:26:20 by mvillarr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>

int main(int ac, char**av)
{
	if (ac != 3)
	{
		std::cout<<"\033[31mERROR: wrong arguments"<<std::endl;
		std::cout<<"Usage : ./ircserv <port> <password>"<<std::endl;
		return 1;
	}
}
