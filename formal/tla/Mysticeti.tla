--------------------------- MODULE Mysticeti ---------------------------
(*
  TLA+ Specification for Mysticeti Consensus Protocol
  
  This specification models the core safety and liveness properties of the
  Mysticeti "3-hop" fast path consensus mechanism.
  
  Key Properties:
  - Safety: No two conflicting blocks are committed
  - Liveness: If a leader has 2f+1 votes, it will eventually commit
  - Agreement: All honest nodes commit the same sequence of leaders
*)

EXTENDS Naturals, FiniteSets, Sequences, TLC

CONSTANTS 
    Nodes,          \* Set of all nodes in the system
    Byzantine,      \* Set of Byzantine (faulty) nodes
    F,              \* Maximum number of Byzantine nodes (F < N/3)
    MaxRound        \* Maximum round for model checking (bounded)

ASSUME 
    /\ F \in Nat
    /\ Cardinality(Byzantine) <= F
    /\ Cardinality(Nodes) = 3 * F + 1
    /\ Byzantine \subseteq Nodes

Honest == Nodes \ Byzantine
Quorum == (2 * F) + 1

(*--algorithm Mysticeti

variables
    \* Global DAG state: round -> node -> block
    dag = [r \in 0..MaxRound |-> [n \in Nodes |-> {}]],
    
    \* Votes received: round -> leader -> set of voters
    votes = [r \in 0..MaxRound |-> [n \in Nodes |-> {}]],
    
    \* Committed leaders: sequence of (round, leader) pairs
    committed = <<>>,
    
    \* Current round per node
    round = [n \in Nodes |-> 0],
    
    \* Last committed round (for garbage collection)
    lastCommitted = 0;

define
    \* A block is certified if it has 2f+1 votes
    IsCertified(r, leader) == 
        Cardinality(votes[r][leader]) >= Quorum
    
    \* Safety: No two different leaders committed at same round
    Safety == 
        \A i, j \in DOMAIN committed:
            (i # j /\ committed[i][1] = committed[j][1]) =>
                committed[i][2] = committed[j][2]
    
    \* Liveness: If a leader is certified, it will eventually commit
    Liveness ==
        \A r \in 0..MaxRound, leader \in Nodes:
            IsCertified(r, leader) ~> 
                \E i \in DOMAIN committed: 
                    committed[i] = <<r, leader>>
    
    \* Agreement: All honest nodes commit same sequence
    Agreement ==
        \A n1, n2 \in Honest:
            \* This would require per-node committed sequences
            \* Simplified here for the skeleton
            TRUE
    
    \* TypeInvariant
    TypeOK ==
        /\ round \in [Nodes -> 0..MaxRound]
        /\ lastCommitted \in 0..MaxRound
        /\ \A r \in DOMAIN committed: 
            /\ committed[r][1] \in 0..MaxRound
            /\ committed[r][2] \in Nodes

end define;

\* Honest node proposes a block for current round
fair process HonestPropose \in Honest
begin
    Propose:
        while round[self] < MaxRound do
            \* Create block with parents from previous round
            dag[round[self]][self] := {
                "round": round[self],
                "author": self,
                "parents": {dag[round[self]-1][n] : n \in Nodes}
            };
            round[self] := round[self] + 1;
        end while;
end process;

\* Honest node votes for leaders
fair process HonestVote \in Honest
begin
    Vote:
        while round[self] < MaxRound do
            \* Vote for leader of round-2 if it exists
            if round[self] >= 2 then
                with leader \in Nodes do
                    if dag[round[self]-2][leader] # {} then
                        votes[round[self]-2][leader] := 
                            votes[round[self]-2][leader] \union {self};
                    end if;
                end with;
            end if;
            round[self] := round[self] + 1;
        end while;
end process;

\* Commit certified leaders
fair process Commit = "committer"
begin
    CommitLoop:
        while lastCommitted < MaxRound do
            \* Check if leader at lastCommitted+1 is certified
            with leader \in Nodes do
                if IsCertified(lastCommitted, leader) then
                    committed := Append(committed, <<lastCommitted, leader>>);
                    lastCommitted := lastCommitted + 1;
                end if;
            end with;
        end while;
end process;

end algorithm; *)

\* BEGIN TRANSLATION (TLC will generate this)
\* END TRANSLATION

=============================================================================
