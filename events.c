#include "sst.h"
#include <math.h>

void events(void) {

	int ictbeam=0, ipage=0, istract=0, line, i, j, k, l, ixhold, iyhold;
	double fintim = state.date + Time, datemin, xtime, repair, yank;
	

#ifdef DEBUG
	if (idebug) prout("EVENTS");
#endif

	if (stdamtim == 1e30 && damage[DRADIO] != 0.0) {
		/* chart will no longer be updated because radio is dead */
		stdamtim = state.date;
		for (i=1; i <= 8 ; i++)
			for (j=1; j <= 8; j++)
				if (starch[i][j] == 1) starch[i][j] = state.galaxy[i][j]+1000;
	}

	for (;;) {
		/* Select earliest extraneous event, line==0 if no events */
		line = FSPY;
		if (alldone) return;
		datemin = fintim;
		for (l=1; l<=NEVENTS; l++)
			if (future[l] <= datemin) {
				line = l;
				datemin = future[l];
			}
		xtime = datemin-state.date;
		state.date = datemin;
		/* Decrement Federation resources and recompute remaining time */
		state.remres -= (state.remkl+4*state.remcom)*xtime;
		state.remtime = state.remres/(state.remkl+4*state.remcom);
		if (state.remtime <=0) {
			finish(FDEPLETE);
			return;
		}
		/* Is life support adequate? */
		if (damage[DLIFSUP] && condit != IHDOCKED) {
			if (lsupres < xtime && damage[DLIFSUP] > lsupres) {
				finish(FLIFESUP);
				return;
			}
			lsupres -= xtime;
			if (damage[DLIFSUP] <= xtime) lsupres = inlsr;
		}
		/* Fix devices */
		repair = xtime;
		if (condit == IHDOCKED) repair /= docfac;
		/* Don't fix Deathray here */
		for (l=1; l<=ndevice; l++)
			if (damage[l] > 0.0 && l != DDRAY)
				damage[l] -= (damage[l]-repair > 0.0 ? repair : damage[l]);
		/* If radio repaired, update star chart and attack reports */
		if (stdamtim != 1e30 && damage[DRADIO] == 0.0) {
			stdamtim = 1e30;
			prout("Lt. Uhura- \"Captain, the sub-space radio is working and");
			prout("   surveillance reports are coming in.");
			skip(1);
			for (i=1; i <= 8 ; i++)
				for (j=1; j <= 8; j++)
					if (starch[i][j] > 999) starch[i][j] = 1;
			if (iseenit==0) {
				attakreport();
				iseenit = 1;
			}
			skip(1);
			prout("   The star chart is now up to date.\"");
			skip(1);
		}
		/* Cause extraneous event LINE to occur */
		Time -= xtime;
		switch (line) {
			case FSNOVA: /* Supernova */
				if (ipage==0) pause(1);
				ipage=1;
				snova(0,0);
				future[FSNOVA] = state.date + expran(0.5*intime);
				if (state.galaxy[quadx][quady] == 1000) return;
				break;
			case FSPY: /* Check with spy to see if S.C. should tractor beam */
				if (state.nscrem == 0 ||
					ictbeam+istract > 0 ||
					condit==IHDOCKED || isatb==1 || iscate==1) return;
				if (ientesc ||
					(energy < 2000 && torps < 4 && shield < 1250) ||
					(damage[DPHASER]>0 && (damage[DPHOTON]>0 || torps < 4)) ||
					(damage[DSHIELD] > 0 &&
					 (energy < 2500 || damage[DPHASER] > 0) &&
					 (torps < 5 || damage[DPHOTON] > 0))) {
					/* Tractor-beam her! */
					istract=1;
					yank = square(state.isx-quadx) + square(state.isy-quady);
					/*********TBEAM CODE***********/
				}
				else return;
			case FTBEAM: /* Tractor beam */
				if (line==FTBEAM) {
					if (state.remcom == 0) {
						future[FTBEAM] = 1e30;
						break;
					}
					i = Rand()*state.remcom+1.0;
					yank = square(state.cx[i]-quadx) + square(state.cy[i]-quady);
					if (istract || condit == IHDOCKED || yank == 0) {
						/* Drats! Have to reschedule */
						future[FTBEAM] = state.date + Time +
										 expran(1.5*intime/state.remcom);
						break;
					}
				}
				/* tractor beaming cases merge here */
				yank = sqrt(yank);
				if (ipage==0) pause(1);
				ipage=1;
				Time = (10.0/(7.5*7.5))*yank; /* 7.5 is yank rate (warp 7.5) */
				ictbeam = 1;
				skip(1);
				proutn("***");
				crmshp();
				prout(" caught in long range tractor beam--");
				/* If Kirk & Co. screwing around on planet, handle */
				atover(1); /* atover(1) is Grab */
				if (alldone) return;
				if (icraft == 1) { /* Caught in Galileo? */
					finish(FSTRACTOR);
					return;
				}
				/* Check to see if shuttle is aboard */
				if (iscraft==0) {
					skip(1);
					if (Rand() >0.5) {
						prout("Galileo, left on the planet surface, is captured");
						prout("by aliens and made into a flying McDonald's.");
						damage[DSHUTTL] = -10;
						iscraft = -1;
					}
					else {
						prout("Galileo, left on the planet surface, is well hidden.");
					}
				}
				if (line==0) {
					quadx = state.isx;
					quady = state.isy;
				}
				else {
					quadx = state.cx[i];
					quady = state.cy[i];
				}
				iran10(&sectx, &secty);
				crmshp();
				proutn(" is pulled to");
				cramlc(1, quadx, quady);
				proutn(", ");
				cramlc(2, sectx, secty);
				skip(1);
				if (resting) {
					prout("(Remainder of rest/repair period cancellestate.)");
					resting = 0;
				}
				if (shldup==0) {
					if (damage[DSHIELD]==0 && shield > 0) {
						doshield(2); /* Shldsup */
						shldchg=0;
					}
					else prout("(Shields not currently useable.)");
				}
				newqad(0);
				/* Adjust finish time to time of tractor beaming */
				fintim = state.date+Time;
				if (state.remcom <= 0) future[FTBEAM] = 1e30;
				else future[FTBEAM] = state.date+Time+expran(1.5*intime/state.remcom);
				break;
			case FSNAP: /* Snapshot of the universe (for time warp) */
				snapsht = state;
				state.snap = 1;
				future[FSNAP] = state.date + expran(0.5 * intime);
				break;
			case FBATTAK: /* Commander attacks starbase */
				if (state.remcom==0 || state.rembase==0) {
					/* no can do */
					future[FBATTAK] = future[FCDBAS] = 1e30;
					break;
				}
				i = 0;
				for (j=1; j<=state.rembase; j++) {
					for (k=1; k<=state.remcom; k++)
						if (state.baseqx[j]==state.cx[k] && state.baseqy[j]==state.cy[k] &&
							(state.baseqx[j]!=quadx || state.baseqy[j]!=quady) &&
							(state.baseqx[j]!=state.isx || state.baseqy[j]!=state.isy)) {
							i = 1;
							break;
						}
					if (i == 1) break;
				}
				if (j>state.rembase) {
					/* no match found -- try later */
					future[FBATTAK] = state.date + expran(0.3*intime);
					future[FCDBAS] = 1e30;
					break;
				}
				/* commander + starbase combination found -- launch attack */
				batx = state.baseqx[j];
				baty = state.baseqy[j];
				future[FCDBAS] = state.date+1.0+3.0*Rand();
				if (isatb) /* extra time if SC already attacking */
					future[FCDBAS] += future[FSCDBAS]-state.date;
				future[FBATTAK] = future[FCDBAS] +expran(0.3*intime);
				iseenit = 0;
				if (damage[DRADIO] != 0.0 &&
					condit != IHDOCKED) break; /* No warning :-( */
				iseenit = 1;
				if (ipage==0) pause(1);
				ipage = 1;
				skip(1);
				proutn("Lt. Uhura-  \"Captain, the starbase in");
				cramlc(1, batx, baty);
				skip(1);
				prout("   reports that it is under atttack and that it can");
				proutn("   hold out only until stardate ");
				cramf(future[FCDBAS],1,1);
				prout(".\"");
				if (resting) {
					skip(1);
					proutn("Mr. Spock-  \"Captain, shall we cancel the rest period?\"");
					if (ja()) {
						resting = 0;
						Time = 0.0;
						return;
					}
				}
				break;
			case FSCDBAS: /* Supercommander destroys base */
				future[FSCDBAS] = 1e30;
				isatb = 2;
				if (state.galaxy[state.isx][state.isy]%100 < 10) break; /* WAS RETURN! */
				ixhold = batx;
				iyhold = baty;
				batx = state.isx;
				baty = state.isy;
			case FCDBAS: /* Commander succeeds in destroying base */
				if (line==FCDBAS) {
					future[FCDBAS] = 1e30;
					/* find the lucky pair */
					for (i = 1; i <= state.remcom; i++)
						if (state.cx[i]==batx && state.cy[i]==baty) break;
					if (i > state.remcom || state.rembase == 0 ||
						state.galaxy[batx][baty] % 100 < 10) {
						/* No action to take after all */
						batx = baty = 0;
						break;
					}
				}
				/* Code merges here for any commander destroying base */
				/* Not perfect, but will have to do */
				if (starch[batx][baty] == -1) starch[batx][baty] = 0;
				/* Handle case where base is in same quadrant as starship */
				if (batx==quadx && baty==quady) {
					if (starch[batx][baty] > 999) starch[batx][baty] -= 10;
					quad[basex][basey]= IHDOT;
					basex=basey=0;
					newcnd();
					skip(1);
					prout("Spock-  \"Captain, I believe the starbase has been destroyestate.\"");
				}
				else if (state.rembase != 1 &&
						 (damage[DRADIO] <= 0.0 || condit == IHDOCKED)) {
					/* Get word via subspace radio */
					if (ipage==0) pause(1);
					ipage = 1;
					skip(1);
					prout("Lt. Uhura-  \"Captain, Starfleet Command reports that");
					proutn("   the starbase in");
					cramlc(1, batx, baty);
					prout(" has been destroyed by");
					if (isatb==2) prout("the Klingon Super-Commander");
					else prout("a Klingon Commander");
				}
				/* Remove Starbase from galaxy */
				state.galaxy[batx][baty] -= 10;
				for (i=1; i <= state.rembase; i++)
					if (state.baseqx[i]==batx && state.baseqy[i]==baty) {
						state.baseqx[i]=state.baseqx[state.rembase];
						state.baseqy[i]=state.baseqy[state.rembase];
					}
				state.rembase--;
				if (isatb == 2) {
					/* reinstate a commander's base attack */
					batx = ixhold;
					baty = iyhold;
					isatb = 0;
				}
				else {
					batx = baty = 0;
				}
				break;
			case FSCMOVE: /* Supercommander moves */
				future[FSCMOVE] = state.date+0.2777;
				if (ientesc+istract==0 &&
					isatb!=1 &&
					(iscate!=1 || justin==1)) scom(&ipage);
				break;
			case FDSPROB: /* Move deep space probe */
				future[FDSPROB] = state.date + 0.01;
				probex += probeinx;
				probey += probeiny;
				i = (int)(probex/10 +0.05);
				j = (int)(probey/10 + 0.05);
				if (probecx != i || probecy != j) {
					probecx = i;
					probecy = j;
					if (i < 1 || i > 8 || j < 1 || j > 8 ||
						state.galaxy[probecx][probecy] == 1000) {
						// Left galaxy or ran into supernova
						if (damage[DRADIO]==0.0 || condit == IHDOCKED) {
							if (ipage==0) pause(1);
							ipage = 1;
							skip(1);
							proutn("Lt. Uhura-  \"The deep space probe ");
							if (i < 1 ||i > 8 || j < 1 || j > 8)
								proutn("has left the galaxy");
							else
								proutn("is no longer transmitting");
							prout(".\"");
						}
						future[FDSPROB] = 1e30;
						break;
					}
					if (damage[DRADIO]==0.0   || condit == IHDOCKED) {
						if (ipage==0) pause(1);
						ipage = 1;
						skip(1);
						proutn("Lt. Uhura-  \"The deep space probe is now in ");
						cramlc(1, probecx, probecy);
						prout(".\"");
					}
				}
				/* Update star chart if Radio is working or have access to
				   radio. */
				if (damage[DRADIO] == 0.0 || condit == IHDOCKED)
					starch[probecx][probecy] = damage[DRADIO] > 0.0 ?
										   state.galaxy[probecx][probecy]+1000 : 1;
				proben--; // One less to travel
				if (proben == 0 && isarmed &&
					state.galaxy[probecx][probecy] % 10 > 0) {
					/* lets blow the sucker! */
					snova(1,0);
					future[FDSPROB] = 1e30;
					if (state.galaxy[quadx][quady] == 1000) return;
				}
				break;
		}
	}
}

				
void wait(void) {
	int key;
	double temp, delay, origTime;

	ididit = 0;
	for (;;) {
		key = scan();
		if (key  != IHEOL) break;
		proutn("How long? ");
	}
	chew();
	if (key != IHREAL) {
		huh();
		return;
	}
	origTime = delay = aaitem;
	if (delay <= 0.0) return;
	if (delay >= state.remtime || nenhere != 0) {
		prout("Are you sure? ");
		if (ja() == 0) return;
	}

	/* Alternate resting periods (events) with attacks */

	resting = 1;
	do {
		if (delay <= 0) resting = 0;
		if (resting == 0) {
			cramf(state.remtime, 0, 2);
			prout(" stardates left.");
			return;
		}
		temp = Time = delay;

		if (nenhere) {
			double rtime = 1.0 + Rand();
			if (rtime < temp) temp = rtime;
			Time = temp;
		}
		if (Time < delay) attack(0);
		if (nenhere==0) movetho();
		if (alldone) return;
		events();
		ididit = 1;
		if (alldone) return;
		delay -= temp;
		/* Repair Deathray if long rest at starbase */
		if (origTime-delay >= 9.99 && condit == IHDOCKED)
			damage[DDRAY] = 0.0;
	} while (state.galaxy[quadx][quady] != 1000); // leave if quadrant supernovas

	resting = 0;
	Time = 0;
}

void nova(int ix, int iy) {
	static double course[] =
		{0.0, 10.5, 12.0, 1.5, 9.0, 0.0, 3.0, 7.5, 6.0, 4.5};
	int bot, top, top2, burst, hits[11][3], kount, icx, icy, mm, nn, j;
	int iquad, iquad1, i, ll, newcx, newcy, ii, jj;
	if (Rand() < 0.05) {
		/* Wow! We've supernova'ed */
		snova(ix, iy);
		return;
	}

	/* handle initial nova */
	quad[ix][iy] = IHDOT;
	crmena(1, IHSTAR, 2, ix, iy);
	prout(" novas.");
	state.galaxy[quadx][quady] -= 1;
	state.starkl++;
	
	/* Set up stack to recursively trigger adjacent stars */
	bot = top = top2 = 1;
	kount = 0;
	icx = icy = 0;
	hits[1][1] = ix;
	hits[1][2] = iy;
	while (1) {
		for (mm = bot; mm <= top; mm++) 
		for (nn = 1; nn <= 3; nn++)  /* nn,j represents coordinates around current */
			for (j = 1; j <= 3; j++) {
				if (j==2 && nn== 2) continue;
				ii = hits[mm][1]+nn-2;
				jj = hits[mm][2]+j-2;
				if (ii < 1 || ii > 10 || jj < 1 || jj > 10) continue;
				iquad = quad[ii][jj];
				switch (iquad) {
//					case IHDOT:	/* Empty space ends reaction
//					case IHQUEST:
//					case IHBLANK:
//					case IHT:
//					case IHWEB:
					default:
						break;
					case IHSTAR: /* Affect another star */
						if (Rand() < 0.05) {
							/* This star supernovas */
							snova(ii,jj);
							return;
						}
						top2++;
						hits[top2][1]=ii;
						hits[top2][2]=jj;
						state.galaxy[quadx][quady] -= 1;
						state.starkl++;
						crmena(1, IHSTAR, 2, ii, jj);
						prout(" novas.");
						quad[ii][jj] = IHDOT;
						break;
					case IHP: /* Destroy planet */
						state.newstuf[quadx][quady] -= 1;
						state.nplankl++;
						crmena(1, IHP, 2, ii, jj);
						prout(" destroyed.");
						state.plnets[iplnet] = nulplanet;
						iplnet = plnetx = plnety = 0;
						if (landed == 1) {
							finish(FPNOVA);
							return;
						}
						quad[ii][jj] = IHDOT;
						break;
					case IHB: /* Destroy base */
						state.galaxy[quadx][quady] -= 10;
						for (i = 1; i <= state.rembase; i++)
							if (state.baseqx[i]==quadx && state.baseqy[i]==quady) break;
						state.baseqx[i] = state.baseqx[state.rembase];
						state.baseqy[i] = state.baseqy[state.rembase];
						state.rembase--;
						basex = basey = 0;
						state.basekl++;
						newcnd();
						crmena(1, IHB, 2, ii, jj);
						prout(" destroyed.");
						quad[ii][jj] = IHDOT;
						break;
					case IHE: /* Buffet ship */
					case IHF:
						prout("***Starship buffeted by nova.");
						if (shldup) {
							if (shield >= 2000.0) shield -= 2000.0;
							else {
								double diff = 2000.0 - shield;
								energy -= diff;
								shield = 0.0;
								shldup = 0;
								prout("***Shields knocked out.");
								damage[DSHIELD] += 0.005*damfac*Rand()*diff;
							}
						}
						else energy -= 2000.0;
						if (energy <= 0) {
							finish(FNOVA);
							return;
						}
						/* add in course nova contributes to kicking starship*/
						icx += sectx-hits[mm][1];
						icy += secty-hits[mm][2];
						kount++;
						break;
					case IHK: /* kill klingon */
						deadkl(ii,jj,iquad, ii, jj);
						break;
					case IHC: /* Damage/destroy big enemies */
					case IHS:
					case IHR:
						for (ll = 1; ll <= nenhere; ll++)
							if (kx[ll]==ii && ky[ll]==jj) break;
						kpower[ll] -= 800.0; /* If firepower is lost, die */
						if (kpower[ll] <= 0.0) {
							deadkl(ii, jj, iquad, ii, jj);
							break;
						}
						newcx = ii + ii - hits[mm][1];
						newcy = jj + jj - hits[mm][2];
						crmena(1, iquad, 2, ii, jj);
						proutn(" damaged");
						if (newcx<1 || newcx>10 || newcy<1 || newcy>10) {
							/* can't leave quadrant */
							skip(1);
							break;
						}
						iquad1 = quad[newcx][newcy];
						if (iquad1 == IHBLANK) {
							proutn(", blasted into ");
							crmena(0, IHBLANK, 2, newcx, newcy);
							skip(1);
							deadkl(ii, jj, iquad, newcx, newcy);
							break;
						}
						if (iquad1 != IHDOT) {
							/* can't move into something else */
							skip(1);
							break;
						}
						proutn(", buffeted to");
						cramlc(2, newcx, newcy);
						quad[ii][jj] = IHDOT;
						quad[newcx][newcy] = iquad;
						kx[ll] = newcx;
						ky[ll] = newcy;
						kavgd[ll] = sqrt(square(sectx-newcx)+square(secty-newcy));
						kdist[ll] = kavgd[ll];
						skip(1);
						break;
				}
			}
		if (top == top2) break;
		bot = top + 1;
		top = top2;
	}
	if (kount==0) return;

	/* Starship affected by nova -- kick it away. */
	dist = kount*0.1;
	if (icx) icx = (icx < 0 ? -1 : 1);
	if (icy) icy = (icy < 0 ? -1 : 1);
	direc = course[3*(icx+1)+icy+2];
	if (direc == 0.0) dist = 0.0;
	if (dist == 0.0) return;
	Time = 10.0*dist/16.0;
	skip(1);
	prout("Force of nova displaces starship.");
	iattak=2;	/* Eliminates recursion problem */
	move();
	Time = 10.0*dist/16.0;
	return;
}
	
	
void snova(int insx, int insy) {
	int comdead, nqx, nqy, nsx, nsy, num, kldead, iscdead;
	int nrmdead, npdead;
	int insipient=0;

	nsx = insy;
	nsy = insy;

	if (insy== 0) {
		if (insx == 1) {
			/* NOVAMAX being used */
			nqx = probecx;
			nqy = probecy;
		}
		else {
			int stars = 0;
			/* Scheduled supernova -- select star */
			/* logic changed here so that we won't favor quadrants in top
			left of universe */
			for (nqx = 1; nqx<=8; nqx++) {
				for (nqy = 1; nqy<=8; nqy++) {
					stars += state.galaxy[nqx][nqy] % 10;
				}
			}
			if (stars == 0) return; /* nothing to supernova exists */
			num = Rand()*stars + 1;
			for (nqx = 1; nqx<=8; nqx++) {
				for (nqy = 1; nqy<=8; nqy++) {
					num -= state.galaxy[nqx][nqy] % 10;
					if (num <= 0) break;
				}
				if (num <=0) break;
			}
#ifdef DEBUG
			if (idebug) {
				proutn("Super nova here?");
				if (ja()==1) {
					nqx = quadx;
					nqy = quady;
				}
			}
#endif
		}

		if (nqx != quady || nqy != quady || justin != 0) {
			/* it isn't here, or we just entered (treat as inroute) */
			if (damage[DRADIO] == 0.0 || condit == IHDOCKED) {
				skip(1);
				proutn("Message from Starfleet Command       Stardate ");
				cramf(state.date, 0, 1);
				skip(1);
				proutn("     Supernova in");
				cramlc(1, nqx, nqy);
				prout("; caution advised.");
			}
		}
		else {
			/* we are in the quadrant! */
			insipient = 1;
			num = Rand()* (state.galaxy[nqx][nqy]%10) + 1;
			for (nsx=1; nsx < 10; nsx++) {
				for (nsy=1; nsy < 10; nsy++) {
					if (quad[nsx][nsy]==IHSTAR) {
						num--;
						if (num==0) break;
					}
				}
				if (num==0) break;
			}
		}
	}
	else {
		insipient = 1;
	}

	if (insipient) {
		skip(1);
		prouts("***RED ALERT!  RED ALERT!");
		skip(1);
		proutn("***Incipient supernova detected at");
		cramlc(2, nsx, nsy);
		skip(1);
		nqx = quadx;
		nqy = quady;
		if (square(nsx-sectx) + square(nsy-secty) <= 2.1) {
			proutn("Emergency override attempts t");
			prouts("***************");
			skip(1);
			stars();
			alldone=1;
		}
	}
	/* destroy any Klingons in supernovaed quadrant */
	num=state.galaxy[nqx][nqy];
	kldead = num/100;
	comdead = iscdead = 0;
	if (nqx==state.isx && nqy == state.isy) {
		/* did in the Supercommander! */
		state.nscrem = state.isx = state.isy = isatb = iscate = 0;
		iscdead = 1;
		future[FSCMOVE] = future[FSCDBAS] = 1e30;
	}
	state.remkl -= kldead;
	if (state.remcom) {
		int maxloop = state.remcom, l;
		for (l = 1; l <= maxloop; l++) {
			if (state.cx[l] == nqx && state.cy[l] == nqy) {
				state.cx[l] = state.cx[state.remcom];
				state.cy[l] = state.cy[state.remcom];
				state.cx[state.remcom] = state.cy[state.remcom] = 0;
				state.remcom--;
				kldead--;
				comdead++;
				if (state.remcom==0) future[FTBEAM] = 1e30;
				break;
			}
		}
	}
	/* destroy Romulans and planets in supernovaed quadrant */
	num = state.newstuf[nqx][nqy];
	state.newstuf[nqx][nqy] = 0;
	nrmdead = num/10;
	state.nromrem -= nrmdead;
	npdead = num - nrmdead*10;
	if (npdead) {
		int l;
		for (l = 1; l <= inplan; l++)
			if (state.plnets[l].x == nqx && state.plnets[l].y == nqy) {
				state.plnets[l] = nulplanet;
			}
	}
	/* Destroy any base in supernovaed quadrant */
	if (state.rembase) {
		int maxloop = state.rembase, l;
		for (l = 1; l <= maxloop; l++)
			if (state.baseqx[l]==nqx && state.baseqy[l]==nqy) {
				state.baseqx[l] = state.baseqx[state.rembase];
				state.baseqy[l] = state.baseqy[state.rembase];
				state.baseqx[state.rembase] = state.baseqy[state.rembase] = 0;
				state.rembase--;
				break;
			}
	}
	/* If starship caused supernova, tally up destruction */
	if (insx) {
		num = state.galaxy[nqx][nqy] % 100;
		state.starkl += num % 10;
		state.basekl += num/10;
		state.killk += kldead;
		state.killc += comdead;
		state.nromkl += nrmdead;
		state.nplankl += npdead;
		state.nsckill += iscdead;
	}
	/* mark supernova in galaxy and in star chart */
	if ((quadx == nqx && quady == nqy) ||
		damage[DRADIO] == 0 ||
		condit == IHDOCKED)
		starch[nqx][nqy] = 1;
	state.galaxy[nqx][nqy] = 1000;
	/* If supernova destroys last klingons give special message */
	if (state.remkl==0 && (nqx != quadx || nqy != quady)) {
		skip(2);
		if (insx == 0) prout("Lucky you!");
		proutn("A supernova in");
		cramlc(1, nqx, nqy);
		prout(" has just destroyed the last Klingons.");
		finish(FWON);
		return;
	}
	/* if some Klingons remain, continue or die in supernova */
	if (alldone) finish(FSNOVAED);
	return;
}
		
				
